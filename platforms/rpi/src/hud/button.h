#pragma once

#include "gl/shaderProgram.h"

#include "shapes.h"
#include "glm/glm.hpp"
#include "rectangle.h"

class Button : public Rectangle {
public:

    void init();
    void draw();

private:
    std::shared_ptr<Tangram::ShaderProgram>  m_fixShader;
    std::shared_ptr<HudMesh>        m_fixMesh;
};
