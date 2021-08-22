#pragma once

#include "gl/shaderProgram.h"

#include "shapes.h"
#include "glm/glm.hpp"
#include "rectangle.h"

namespace Tangram {
	class Map;
}

class SlideZoom : public Rectangle {
public:

    void init();
    void draw(Tangram::RenderState& rs, std::unique_ptr<Tangram::Map>& pMap);

    float zoom;
    float slider;

private:
    std::shared_ptr<Tangram::ShaderProgram>  m_fixShader;
    std::shared_ptr<Tangram::ShaderProgram>  m_trnShader;

    std::shared_ptr<HudMesh>        m_verticalRulerMeshA;
    std::shared_ptr<HudMesh>        m_verticalRulerMeshB;
    std::shared_ptr<HudMesh>        m_triangle;
    std::shared_ptr<HudMesh>        m_fixed;
};
