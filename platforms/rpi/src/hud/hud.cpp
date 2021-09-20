#include "hud.h"
#include "map.h"
#include <iostream>
#include "context.h"

#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"

#include "gl/vertexLayout.h"
#include "util/geom.h"

#include "tangram.h"

#include "view/view.h"

#include "../gps.h"

#include "wrap.h"
#include "gl/renderState.h"

#define STRINGIFY(A) #A

Hud::Hud(): m_selected(0), m_bCursor(true){
}

Hud::~Hud(){
}

void Hud::setDrawCursor(bool _true){
    m_bCursor = _true;
}

void Hud::init() {
    std::cout << "Window width = " << getWindowWidth() << ", Window height = " << getWindowHeight() << std::endl;
    m_zoom.set(getWindowWidth()*0.9575,getWindowHeight()*0.30625,getWindowWidth()*0.02625,getWindowHeight()*0.3875);
    m_zoom.init();

    m_rot.set(getWindowWidth()*0.34625,getWindowHeight()*0.866667,getWindowWidth()*0.3125,getWindowHeight()*0.1);
    m_rot.init();
    
    //m_rot.set(getWindowWidth()*0.34625,getWindowHeight()*0.05,getWindowWidth()*0.3125,getWindowHeight()*0.1);
    //m_rot.init();

    m_center.set(getWindowWidth()*0.93625,getWindowHeight()*0.8958,getWindowHeight()*0.0708,getWindowHeight()*0.0708);
    m_center.init();
    
    m_speed.set(0, 0, getWindowWidth(), getWindowHeight());
    m_speed.init();
    m_speed.setFont("beteckna/BetecknaGSCondensed-Bold.ttf", 72);
    m_speed.setText("55");
    
    //if (m_bCursor){
        std::string frag =
"#ifdef GL_ES\n"
"precision mediump float;\n"
"#endif\n";

        // Translatation Vertex Shader
        std::string trnVertShader = frag;
        trnVertShader += STRINGIFY(
        uniform mat4 u_modelViewProjectionMatrix;
        uniform vec2 u_offset;
        attribute vec4 a_position;
        varying vec4 v_position;
        void main(void) {
            v_position = vec4(u_offset.x,u_offset.y,0.1,0.0) + a_position;
            gl_Position = u_modelViewProjectionMatrix * v_position;
        });

        // Universal Fragment Shader
        frag += STRINGIFY(
        uniform vec2 u_resolution;
        uniform vec4 u_color;
        void main(void) {
            gl_FragColor = vec4(1.0);
        });

        m_trnShader = std::shared_ptr<Tangram::ShaderProgram>(new Tangram::ShaderProgram());
        m_trnShader->setShaderSource(trnVertShader, frag);

        m_cursorMesh = getCrossMesh(10.0f);
    //}
}

void Hud::cursorClick(float _x, float _y, int _button, std::unique_ptr<Tangram::Map>& pMap){
    std::cout << "CURSOR CLICK" << std::endl;
    if (m_center.inside(_x,_y)){
        float lat = 0.0;
        float lon = 0.0; 
        if (getLocation(&lat,&lon)){
            // GO TO CENTER
            std::cout << "GO TO " << lat << " lat, " << lon << " lon"<< std::endl;
            pMap->setPosition(lon, lat);
            //Tangram::setViewPosition(lon,lat);
        } else {
            std::cout << "NO FIX GPS" << std::endl;  
        }
    } else if (m_rot.inside(_x,_y)){
        m_selected = 1;
    } else if (m_zoom.inside(_x,_y)){
        m_selected = 2;
    }
}

void Hud::cursorDrag(float _x, float _y, int _button, std::unique_ptr<Tangram::Map>& pMap){
    std::cout << "CURSOR DRAG" << std::endl;
    if (m_selected == 1) {

        float scale = -1.0;

        glm::vec2 screen = glm::vec2(getWindowWidth(),getWindowHeight());
        glm::vec2 mouse = glm::vec2(_x,_y)-screen*glm::vec2(0.5);
        glm::vec2 prevMouse = mouse - glm::vec2(getMouseVelX(),getMouseVelY());

        double rot1 = atan2(mouse.y, mouse.x);
        double rot2 = atan2(prevMouse.y, prevMouse.x);
        double rot = rot1-rot2;
        rot = WrapTwoPI(rot);
        pMap->handleRotateGesture(getWindowWidth()/2.0, getWindowHeight()/2.0, rot*scale);

    } else if (m_selected == 2) {
        
        pMap->handlePinchGesture(getWindowWidth()/2.0, getWindowHeight()/2.0, 1.0 + getMouseVelY()*0.01, 0.0f);
        m_zoom.slider += getMouseVelY();

    }
}

void Hud::cursorRelease(float _x, float _y, std::unique_ptr<Tangram::Map>& pMap){
    m_selected = 0;
}

bool Hud::isInUse(){
    return m_selected != 0;
}

void Hud::draw(std::unique_ptr<Tangram::Map>& pMap){
    
	Tangram::RenderState& rs = pMap->getRenderState();
	GLboolean depthTest = rs.depthTest(GL_FALSE);
	GLboolean blending = rs.blending(GL_TRUE);
	rs.blendingFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Cursor
    if (m_bCursor){
        
        Tangram::UniformLocation trn_u_offset("u_offset");
        Tangram::UniformLocation trn_u_mask("u_mask");
        Tangram::UniformLocation trn_u_resolution("u_resolution");
        Tangram::UniformLocation trn_u_modelViewProjectionMatrix("u_modelViewProjectionMatrix");
        
        std::cout << "use" << std::endl;
        m_trnShader->use(rs);
        
        std::cout << "u_offset" << std::endl;
        m_trnShader->setUniformf(rs, trn_u_offset, getMouseX(), getWindowHeight()-getMouseY());
        
        std::cout << "u_modelViewProjectionMatrix" << std::endl;
        m_trnShader->setUniformMatrix4f(rs, trn_u_modelViewProjectionMatrix, pMap->getView().getOrthoViewportMatrix());
        
        std::cout << "draw" << std::endl;
        m_cursorMesh->draw(rs, *(m_trnShader.get()));
    }
    
    m_speed.draw(rs);
    
    // Zoom
    m_zoom.zoom = pMap->getZoom();
    m_zoom.draw(rs, pMap);
    
    // Rotation
    m_rot.angle = pMap->getRotation();
    m_rot.draw(rs, pMap);

    // Center button
    m_center.draw(rs, pMap);

    rs.depthTest(depthTest);
    rs.blending(blending);
    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
    
    
}
