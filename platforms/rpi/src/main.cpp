#include "context.h"
#include "map.h"
#include "log.h"
#include "rpiPlatform.h"
#include "hud.h"
#include "debug/textDisplay.h"
#include "view/view.h"

#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <iostream>
#include <memory>

#include <fcntl.h>
#include <sys/time.h>
#include <sys/shm.h>
#include <termios.h>
#include "ioboard_if2.h"
#include "gps.h"

Hud hud;

#define KEY_ESC      27     // esc
#define KEY_ZOOM_IN  61     // =
#define KEY_ZOOM_OUT 45     // -
#define KEY_UP       119    // w
#define KEY_LEFT     97     // a
#define KEY_DOWN     115    // s
#define KEY_RIGHT    100    // d
#define KEY_CAMERA   99     // c

using namespace Tangram;

std::unique_ptr<Map> map;

std::string apiKey;

static bool bUpdate = true;

IOBoard* pBoard;

struct LaunchOptions {
    std::string sceneFilePath = "res/scene.yaml";
    double latitude = 0.0;
    double longitude = 0.0;
    int x = 0;
    int y = 0;
    int width = 0;
    int height = 0;
    float zoom = 0.0f;
    float rotation = 0.0f;
    float tilt = 0.0f;
    bool hasLocationSet = false;
    bool drawCursor = false;
};

LaunchOptions getLaunchOptions(int argc, char **argv) {

    LaunchOptions options;

    for (int i = 1; i < argc - 1; i++) {
        std::string argName = argv[i];
        std::string argValue = argv[i + 1];
        if (argName == "-s" || argName == "--scene") {
            options.sceneFilePath = argValue;
        } else if (argName == "-lat" || argName == "--latitude") {
            options.latitude = std::stod(argValue);
            options.hasLocationSet = true;
        } else if (argName == "-lon" || argName == "--longitude") {
            options.longitude = std::stod(argValue);
            options.hasLocationSet = true;
        } else if (argName == "-z" || argName == "--zoom" ) {
            options.zoom = std::stof(argValue);
            options.hasLocationSet = true;
        } else if (argName == "-x" || argName == "--x_position") {
            options.x = std::stoi(argValue);
        } else if (argName == "-y" || argName == "--y_position") {
            options.y = std::stoi(argValue);
        } else if (argName == "-w" || argName == "--width") {
            options.width = std::stoi(argValue);
        } else if (argName == "-h" || argName == "--height") {
            options.height = std::stoi(argValue);
        } else if (argName == "-t" || argName == "--tilt") {
            options.tilt = std::stof(argValue);
        } else if (argName == "-r" || argName == "--rotation") {
            options.rotation = std::stof(argValue);
        } else if (argName == "-m" || argName == "--mouse_cursor") {
            options.drawCursor = true;
        }
    }
    return options;
}

struct Timer {
    timeval start;

    Timer() {
        gettimeofday(&start, NULL);
    }

    // Get the time in seconds since delta was last called.
    double deltaSeconds() {
        timeval current;
        gettimeofday(&current, NULL);
        double delta = (double)(current.tv_sec - start.tv_sec) + (current.tv_usec - start.tv_usec) * 1.0E-6;
        start = current;
        return delta;
    }
};

std::atomic<bool> enableMainScreen;
std::atomic<bool> enableCamera;
std::atomic<float> fuelFrac;

/*
class ToggleScreen : public Subscriber {
public:
    virtual void ioEvent(IOState state) {
        std::cout << "Toggle screen" << std::endl;
        enableMainScreen.store(state.value);
        onKeyPress(KEY_ZOOM_OUT);
    }
};
*/

class ZoomOut : public Subscriber {
public:
    virtual void ioEvent(IOState state) {
        if(state.value == 1) {
            std::cout << "Zoom out screen" << std::endl;
            onKeyPress(KEY_ZOOM_OUT);
        }
    }
};

class ZoomIn : public Subscriber {
public:
    virtual void ioEvent(IOState state) {
        if(state.value == 1) {
            std::cout << "Zoom in screen" << std::endl;
            onKeyPress(KEY_ZOOM_IN);
        }
    }
};

class CameraToggle : public Subscriber {
    virtual void ioEvent(IOState state) {
        if(state.value == 1) {
            std::cout << "Camera toggle" << std::endl;
            onKeyPress(KEY_CAMERA);
        }
    }    
};

class FuelAmount : public Subscriber {
    virtual void ioEvent(IOState state) {
        //if(state.value == 1) {
            //std::cout << "Fuel amount = " << state.value << std::endl;
            fuelFrac.store(((float) state.value) / 255.0f);
            //onKeyPress(KEY_CAMERA);
        //}
    }    
};

int main(int argc, char **argv) {

    printf("Starting an interactive map window. Use keys to navigate:\n"
           "\t'%c' = Pan up\n"
           "\t'%c' = Pan left\n"
           "\t'%c' = Pan down\n"
           "\t'%c' = Pan right\n"
           "\t'%c' = Zoom in\n"
           "\t'%c' = Zoom out\n"
           "\t'esc'= Exit\n"
           "Press 'enter' to continue.",
           KEY_UP, KEY_LEFT, KEY_DOWN, KEY_RIGHT, KEY_ZOOM_IN, KEY_ZOOM_OUT);

    if (getchar() == -1) {
        return 1;
    }

    LaunchOptions options = getLaunchOptions(argc, argv);
    
    GPSService gps;
    std::thread gps_thread = gps.start();
    
    int pi = pigpio_start(0, 0);
    pBoard = new IOBoard(pi, 30);
    
    fuelFrac.store(0.75f);
    enableCamera.store(false);
    
    std::shared_ptr<PrintEvent> pe = std::make_shared<PrintEvent>();
    for(int i = 0; i < 16; i++) {
		pBoard->subscribe(i, pe);
	}
    
    std::thread board_thread = pBoard->spawn();
    
    std::shared_ptr< ZoomIn > zoomIn = std::make_shared<ZoomIn>();
    std::shared_ptr< ZoomOut > zoomOut = std::make_shared<ZoomOut>();
    std::shared_ptr< CameraToggle > cameraOn = std::make_shared<CameraToggle>();
    std::shared_ptr< FuelAmount > fuelAmt = std::make_shared<FuelAmount>();
    
    pBoard->subscribe(12, zoomOut);
    pBoard->subscribe(11, zoomIn);
    pBoard->subscribe(10, cameraOn);
    pBoard->subscribe(16, fuelAmt);
    
    UrlClient::Options urlClientOptions;
    urlClientOptions.maxActiveTasks = 10;

    // Start OpenGL context
    createSurface(options.x, options.y, options.width, options.height);

    std::vector<SceneUpdate> updates;

    // Get Mapzen API key from environment variables.
    char* nextzenApiKeyEnvVar = getenv("NEXTZEN_API_KEY");
    if (nextzenApiKeyEnvVar && strlen(nextzenApiKeyEnvVar) > 0) {
        apiKey = nextzenApiKeyEnvVar;
        updates.push_back(SceneUpdate("global.sdk_api_key", apiKey));
    } else {
        LOGW("No API key found!\n\nNextzen data sources require an API key. "
             "Sign up for a key at https://developers.nextzen.org/about.html and then set it from the command line with: "
             "\n\n\texport NEXTZEN_API_KEY=YOUR_KEY_HERE\n");
    }

    // Resolve the scene file URL against the current directory.
    Url baseUrl("file:///");
    char pathBuffer[PATH_MAX] = {0};
    if (getcwd(pathBuffer, PATH_MAX) != nullptr) {
        baseUrl = baseUrl.resolve(Url(std::string(pathBuffer) + "/"));
    }

    LOG("Base URL: %s", baseUrl.string().c_str());

    Url sceneUrl = baseUrl.resolve(Url(options.sceneFilePath));
    hud.setDrawCursor(options.drawCursor);

    map = std::make_unique<Map>(std::make_unique<RpiPlatform>(urlClientOptions));
    map->loadScene(sceneUrl.string(), !options.hasLocationSet, updates);
    map->setupGL();
    map->resize(getWindowWidth(), getWindowHeight());
    map->setTilt(options.tilt);
    map->setRotation(options.rotation);

    if (options.hasLocationSet) {
        map->setPosition(options.longitude, options.latitude);
        map->setZoom(options.zoom);
    }

    std::cout << "Hud init" << std::endl;
    hud.init();
    
    std::cout << "Hud init cursor" << std::endl;    
    hud.setDrawCursor(true);
    
    TextDisplay::Instance().init();
    
    // Start clock
    Timer timer;

    std::vector< std::string > info;
    info.push_back("test");
    
    unsigned int opacity = 0xFF;
    int cameraPID = 0;
    
    std::cout << "Loop start" << std::endl;
    while (bUpdate) {
        pollInput();
        double dt = timer.deltaSeconds();
        if(enableCamera.load()) {
            if(opacity != 0x00) {
                opacity = 0x00;
                std::cout << "Setting opacity to " << opacity << std::endl;
                setSurfaceOpacity(opacity);
                swapSurface();
                int pid = fork();
                if(pid) {
                    cameraPID = pid;
                }else{
                    std::cout << "starting camera stream" << std::endl;
                    system("gst-launch-1.0 -v tcpclientsrc host=192.168.86.242 port=9000 ! decodebin ! videoconvert ! fbdevsink");
                    std::cout << "stream finished" << std::endl;
                    return 0;
                }
            }
        }else{
            if(opacity != 0xFF) {
                opacity = 0xFF;
                if(cameraPID) {
                    killpg(cameraPID, SIGKILL);
                    system("killall gst-launch-1.0");
                    cameraPID = 0;
                }
                std::cout << "Setting opacity to " << opacity << std::endl;
                setSurfaceOpacity(opacity);
            }
            //enableSurface();
            if (getRenderRequest() || map->getPlatform().isContinuousRendering() ) {
                setRenderRequest(false);
                float lat = gps.getLatitude();
                float lon = gps.getLongitude();
                if(lat != 0 && lon != 0) {
                    map->setPosition(lon, lat);
                }
                map->update(dt);
                hud.setFuel(fuelFrac.load());
                map->render();
                hud.draw(map, gps);
                TextDisplay::Instance().draw(map->getRenderState(), info);
            }
            swapSurface();
        }
        pBoard->processEvents();
    }

    if (map) {
        map = nullptr;
    }

    std::cout << "exit 1" << std::endl;
    
    TextDisplay::Instance().deinit();

    std::cout << "exit 2" << std::endl;

    destroySurface();
    
    std::cout << "exit 3" << std::endl;
    
    pBoard->set(-1, -1);
    
    std::cout << "exit 3a" << std::endl;
    
    board_thread.join();

    std::cout << "exit 3b" << std::endl;
    
    gps.stop();
    gps_thread.join();

    std::cout << "exit 4" << std::endl;
    
    delete pBoard;

    std::cout << "exit 5" << std::endl;
    
    pigpio_stop(pi);

    std::cout << "exit 6" << std::endl;
    
    return 0;
}

//======================================================================= EVENTS

void onKeyPress(int _key) {
    switch (_key) {
        case KEY_ZOOM_IN:
            map->setZoom(map->getZoom() + 1.0f);
            break;
        case KEY_ZOOM_OUT:
            map->setZoom(map->getZoom() - 1.0f);
            break;
        case KEY_UP:
            map->handlePanGesture(0.0, 0.0, 0.0, 100.0);
            break;
        case KEY_DOWN:
            map->handlePanGesture(0.0, 0.0, 0.0, -100.0);
            break;
        case KEY_LEFT:
            map->handlePanGesture(0.0, 0.0, 100.0, 0.0);
            break;
        case KEY_RIGHT:
            map->handlePanGesture(0.0, 0.0, -100.0, 0.0);
            break;
        case KEY_ESC:
            bUpdate = false;
            break;
        case KEY_CAMERA:
            bool cur = enableCamera.load();
            enableCamera.store(!cur);
            break;
        default:
            logMsg(" -> %i\n",_key);
    }
    setRenderRequest(true);
}

void onMouseMove(float _x, float _y) {
    setRenderRequest(true);
}

void onMouseClick(float _x, float _y, int _button) {
    setRenderRequest(true);
}

void onMouseDrag(float _x, float _y, int _button) {
    if( _button == 1 ){
		if (hud.isInUse()){
            hud.cursorDrag(_x,_y,_button, map);
        } else {
        	map->handlePanGesture(_x - getMouseVelX(), _y + getMouseVelY(), _x, _y);
		}
    } else if( _button == 2 ){
        if ( getKeyPressed() == 'r') {
            float scale = -0.05;
            float rot = atan2(getMouseVelY(),getMouseVelX());
            if( _x < getWindowWidth()/2.0 ) {
                scale *= -1.0;
            }
            map->handleRotateGesture(getWindowWidth()/2.0, getWindowHeight()/2.0, rot*scale);
        } else if ( getKeyPressed() == 't') {
            map->handleShoveGesture(getMouseVelY()*0.005);
        } else {
            map->handlePinchGesture(getWindowWidth()/2.0, getWindowHeight()/2.0, 1.0 + getMouseVelY()*0.001, 0.f);
        }

    }
    setRenderRequest(true);
}

void onMouseRelease(float _x, float _y) {
	hud.cursorRelease(_x,_y, map);
    setRenderRequest(true);
}

void onViewportResize(int _newWidth, int _newHeight) {
    setRenderRequest(true);
}
