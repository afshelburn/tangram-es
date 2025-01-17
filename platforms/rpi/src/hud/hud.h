#pragma once

#include "gl/shaderProgram.h"

#include "shapes.h"
#include "glm/glm.hpp"

#include "slideZoom.h"
#include "slideRot.h"
#include "button.h"
#include "hudText.h"
#include "guage.h"

#include "map.h"

class GPSService;

class Hud {
public:

    Hud();
    virtual ~Hud();

    void setDrawCursor(bool _true);

    void init();
    void draw(std::unique_ptr<Tangram::Map>& pMap, GPSService& gps);

    void cursorClick(float _x, float _y, int _button, std::unique_ptr<Tangram::Map>& pMap);
    void cursorDrag(float _x, float _y, int _button, std::unique_ptr<Tangram::Map>& pMap);
    void cursorRelease(float _x, float _y, std::unique_ptr<Tangram::Map>& pMap);

    bool isInUse();
    
    void setFuel(float frac);

    SlideRot    m_rot;
    SlideZoom   m_zoom;
    Button      m_center;
    //Button      m_cursor;
    HudText     m_speed;
    HudText     m_speedUnit;
    HudText     m_clock;
    Guage       m_fuel;


private:

    std::shared_ptr<Tangram::ShaderProgram>  m_trnShader;
    std::shared_ptr<HudMesh>       m_cursorMesh;  

    int     m_selected; 
    bool    m_bCursor; 
};
