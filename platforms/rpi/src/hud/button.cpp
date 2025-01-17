#include "button.h"

#include "context.h"
#include <iostream>
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "glm/gtx/string_cast.hpp"

#include "gl/vertexLayout.h"
#include "util/geom.h"

#include "gl/renderState.h"

#include "map.h"
#include "view/view.h"

void Button::init(){

    std::string frag =
"#ifdef GL_ES\n"
"precision mediump float;\n"
"#endif\n";

    // Fixed Vertex Shader
    std::string fixVertShader = frag;
    fixVertShader += STRINGIFY(
    uniform mat4 u_modelViewProjectionMatrix;
    attribute vec4 a_position;
    varying vec4 v_position;
    varying float v_alpha;
    void main(void) {
        v_position = a_position;
        v_position.z = 0.1;
        v_alpha = 1.0;
        gl_Position = u_modelViewProjectionMatrix * v_position;
    });

    // Universal Fragment Shader
    frag += STRINGIFY(
    uniform vec2 u_resolution;
    uniform vec4 u_mask;
    uniform vec4 u_color;
    varying float v_alpha;
    void main(void) {
        vec2 st = gl_FragCoord.xy;
        vec2 mask = step(u_mask.xy,st)*(1.0-step(u_mask.zw,st));
        gl_FragColor = vec4(vec3(1.0),v_alpha)*(mask.x*mask.y);
    });

    m_fixShader = std::shared_ptr<Tangram::ShaderProgram>(new Tangram::ShaderProgram());
    m_fixShader->setShaderSource(fixVertShader, frag);

    {
        float cornersWidth = 10.;
        float crossWidth = 5.;

        std::shared_ptr<Tangram::VertexLayout> vertexLayout = std::shared_ptr<Tangram::VertexLayout>(new Tangram::VertexLayout({
            {"a_position", 2, GL_FLOAT, false, 0}
        }));
        std::vector<LineVertex> vertices;
        std::vector<short unsigned int> indices;

        // Cross
        glm::vec3 center = getCenter();
        std::cout << "Center = " << glm::to_string(center) << std::endl;
        
        vertices.push_back({ center.x-crossWidth, center.y});
        vertices.push_back({ center.x+crossWidth, center.y});
        vertices.push_back({ center.x, center.y-crossWidth});
        vertices.push_back({ center.x, center.y+crossWidth});
        
        indices.push_back(0); indices.push_back(1); 
        indices.push_back(2); indices.push_back(3);

        // Coorners
        glm::vec3 A = getTopLeft();
        glm::vec3 B = getTopRight();
        glm::vec3 C = getBottomRight();
        glm::vec3 D = getBottomLeft();
        
        std::cout << "Top Left = " << glm::to_string(A) << std::endl;
        std::cout << "Top Right = " << glm::to_string(B) << std::endl;
        std::cout << "Bottom Right = " << glm::to_string(C) << std::endl;
        std::cout << "Bottom Left = " << glm::to_string(D) << std::endl;

        vertices.push_back({ A.x, A.y + cornersWidth});
        vertices.push_back({ A.x, A.y});
        vertices.push_back({ A.x + cornersWidth, A.y});

        vertices.push_back({ B.x - cornersWidth, B.y});
        vertices.push_back({ B.x, B.y});
        vertices.push_back({ B.x, B.y + cornersWidth});

        vertices.push_back({ C.x, C.y - cornersWidth});
        vertices.push_back({ C.x, C.y});
        vertices.push_back({ C.x - cornersWidth, C.y});

        vertices.push_back({ D.x + cornersWidth, D.y});
        vertices.push_back({ D.x, D.y});
        vertices.push_back({ D.x, D.y - cornersWidth});

        indices.push_back(4); indices.push_back(5);
        indices.push_back(5); indices.push_back(6);
        indices.push_back(7); indices.push_back(8);
        indices.push_back(8); indices.push_back(9);
        indices.push_back(10); indices.push_back(11);
        indices.push_back(11); indices.push_back(12);
        indices.push_back(13); indices.push_back(14);
        indices.push_back(14); indices.push_back(15);

        std::shared_ptr<HudMesh> mesh(new HudMesh(vertexLayout, GL_LINES));
        Tangram::MeshData<LineVertex> md;
        md.indices = indices;
        md.vertices = vertices;
        md.offsets.emplace_back(indices.size(), vertices.size());
        mesh->compile(md);
        //mesh->addVertices(std::move(vertices), std::move(indices));
        //mesh->compileVertexBuffer();

        m_fixMesh = mesh;
    } 
}

void Button::draw(Tangram::RenderState& rs, std::unique_ptr<Tangram::Map>& pMap){
    m_fixShader->use(rs);
    Tangram::UniformLocation u_mask("u_mask");
    Tangram::UniformLocation u_modelViewProjectionMatrix("u_modelViewProjectionMatrix");
    m_fixShader->setUniformf(rs, u_mask, 0, 0, getWindowWidth(), getWindowHeight());
    //pMap->getView()->getOrthoMatrix();
    //std::cout << "matrix = " << glm::to_string(pMap->getView().getOrthoViewportMatrix()) << std::endl;
    m_fixShader->setUniformMatrix4f(rs, u_modelViewProjectionMatrix, pMap->getView().getOrthoViewportMatrix());
    m_fixMesh->draw(rs, *(m_fixShader.get()));
}
