#include "gps.h"
#include <ctime>

GPSService::GPSService() : gps("localhost", DEFAULT_GPSD_PORT), update(true) {}
GPSService::~GPSService() {}

std::thread GPSService::start() {
        
    return std::thread(&GPSService::loop, this);

}

void GPSService::stop() {
    this->update.store(false);
}

float GPSService::getLatitude() {
    return this->lat.load();
}

float GPSService::getLongitude() {
    return this->lon.load();
}

float GPSService::getSpeed() {
    return this->speed.load();
}

int GPSService::getHour() {
    return this->hour.load();
}

int GPSService::getMinute() {
    return this->minute.load();
}

void GPSService::loop() {
        
    if (gps.stream(WATCH_ENABLE | WATCH_JSON) == nullptr) {
        std::cerr << "No GPSD running.\n";
        return;
    }
    
    constexpr auto kWaitingTime{1000000};  // 1000000 is 1 second

    for (;;) {
            
        if(!update.load()) {
            return;
        }
        
        if (!gps.waiting(kWaitingTime)) {
            continue;
        }

        struct gps_data_t* gpsd_data;

        if ((gps.read()) == nullptr) {
            std::cerr << "GPSD read error.\n";
            return;
        }

        while (((gpsd_data = gps.read()) == nullptr) || (gpsd_data->fix.mode < MODE_2D)) {
            // Do nothing until fix, block execution for 1 second (busy wait mitigation)
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }

        const auto latitude{gpsd_data->fix.latitude};
        const auto longitude{gpsd_data->fix.longitude};
        const auto hdop{gpsd_data->dop.hdop};
        const auto vdop{gpsd_data->dop.vdop};
        const auto pdop{gpsd_data->dop.pdop};
        const auto s_vis{gpsd_data->satellites_visible};
        const auto s_used{gpsd_data->satellites_used};
        //const auto time_str{TimespecToTimeStr(gpsd_data->fix.time, ISO_8601)};  // you can change the 2nd argument to LOCALTIME, UTC, UNIX or ISO8601

        //std::cout << std::setprecision(8) << std::fixed;  // set output to fixed floating point, 8 decimal precision
        //std::cout << "GPSD:" << gpsd_data->fix.time << "," << latitude << "," << longitude << "," << hdop << "," << vdop << "," << pdop << "," << s_vis << "," << s_used << '\n';
    
        this->lat.store(latitude);
        this->lon.store(longitude);
        this->speed.store(gpsd_data->fix.speed);
        
        
        //get the starting value of clock
        //clock_t start = clock();
        tm* my_time;


        //get current time in format of time_t
        time_t t = gpsd_data->fix.time;//time(NULL);


        //show the value stored in t
        //std::cout << "Value of t " << t << std::endl;

        //convert time_t to char*
        //char* charTime = ctime(&t);

        //display current time
        //std::cout << "Now is " << charTime << std::endl;

        //convert time_t to tm
        my_time = localtime(&t);

        //get only hours and minutes
        //char buf[80] = {0};
        //strftime(buf, 10, "%I:%M%p", my_time);
        //std::string test(buf);
        
        //std::cout << test << std::endl;
        
        this->hour.store(my_time->tm_hour);
        this->minute.store(my_time->tm_min);
    
    
    }
}
