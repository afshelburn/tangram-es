#pragma once

#include "gl/shaderProgram.h"

#include "shapes.h"
#include "glm/glm.hpp"

#include "slideZoom.h"
#include "slideRot.h"
#include "button.h"

class Map;

class Hud {
public:

    Hud();
    virtual ~Hud();

    void setDrawCursor(bool _true);

    void init();
    void draw();

    void cursorClick(float _x, float _y, int _button, Map* pMap);
    void cursorDrag(float _x, float _y, int _button, Map* pMap);
    void cursorRelease(float _x, float _y, Map* pMap);

    bool isInUse();

    SlideRot    m_rot;
    SlideZoom   m_zoom;
    Button      m_center;

private:

    std::shared_ptr<Tangram::ShaderProgram>  m_trnShader;
    std::shared_ptr<HudMesh>       m_cursorMesh;  

    int     m_selected; 
    bool    m_bCursor; 
};
