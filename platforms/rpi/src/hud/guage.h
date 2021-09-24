#pragma once

#include "gl/shaderProgram.h"

#include "shapes.h"
#include "glm/glm.hpp"
#include "rectangle.h"

namespace Tangram {
	class Map;
}

class Guage : public Rectangle {
public:

    void init();
    void draw(Tangram::RenderState& rs, std::unique_ptr<Tangram::Map>& pMap);

    float value;
    
    void setValue(float v) {
		this->value = v;
	}

private:
    std::shared_ptr<Tangram::ShaderProgram>  m_fixShader;

    std::shared_ptr<HudMesh>        m_triangle;
    std::shared_ptr<HudMesh>        m_verticalMajorRulerMesh;
    std::shared_ptr<HudMesh>        m_verticalMinorRulerMesh;
};

