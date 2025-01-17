#include "slideZoom.h"

#include <iostream>
#include "context.h"

#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"

#include "gl/vertexLayout.h"
#include "util/geom.h"

#include "gl/renderState.h"
#include "view/view.h"

#include "map.h"

void SlideZoom::init(){

    std::cout << "zoom init a" << std::endl;
    
    zoom = 0;
    slider = 0;

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

    // Translatation Vertex Shader
    std::string trnVertShader = frag;
    trnVertShader += STRINGIFY(
    uniform mat4 u_modelViewProjectionMatrix;
    uniform vec2 u_offset;
    uniform vec2 u_resolution;
    attribute vec4 a_position;
    varying vec4 v_position;
    varying float v_alpha;
    void main(void) {
        v_position = vec4(u_offset.x,u_offset.y,0.1,0.0) + a_position;
        v_alpha = 1.0;

        float toCenter = v_position.y-u_resolution.y*0.5;
        v_alpha = 1.0-clamp( abs(toCenter/(u_resolution.y*2.)) ,0.0,1.0);

        gl_Position = u_modelViewProjectionMatrix * v_position;
    });

    std::cout << "zoom init b" << std::endl;

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

    std::cout << "zoom init b1" << std::endl;
    m_fixShader = std::shared_ptr<Tangram::ShaderProgram>(new Tangram::ShaderProgram());
    std::cout << "zoom init b2" << std::endl;
    m_fixShader->setShaderSource(fixVertShader, frag);
    
    std::cout << "zoom init b0" << std::endl;
    
    m_trnShader = std::shared_ptr<Tangram::ShaderProgram>(new Tangram::ShaderProgram());
    m_trnShader->setShaderSource(trnVertShader, frag);

    std::cout << "zoom init b1" << std::endl;
    
    m_verticalRulerMeshA = getVerticalRulerMesh(-getWindowHeight(),getWindowHeight(),5.0f,getWindowWidth()*0.0131125);
    m_verticalRulerMeshB = getVerticalRulerMesh(-getWindowHeight(),getWindowHeight(),50.0f,getWindowWidth()*0.0194525);

    m_triangle = getTriangle(getWindowHeight()*0.01);

    std::cout << "zoom init c" << std::endl;
    // Make fixed lines
    {
        std::shared_ptr<Tangram::VertexLayout> vertexLayout = std::shared_ptr<Tangram::VertexLayout>(new Tangram::VertexLayout({
            {"a_position", 2, GL_FLOAT, false, 0}
        }));
        std::vector<LineVertex> vertices;
        std::vector<unsigned short int> indices;

        vertices.push_back({ x - 10.0, getWindowHeight()*0.5-20.0 });
        vertices.push_back({ x - 5.0, getWindowHeight()*0.5-20.0 });
        indices.push_back(0); indices.push_back(1);

        vertices.push_back({ x - 7.5, getWindowHeight()*0.5 });
        vertices.push_back({ x - 5.0, getWindowHeight()*0.5 });        
        indices.push_back(2); indices.push_back(3);

        vertices.push_back({ x - 10.0, getWindowHeight()*0.5+20.0 });
        vertices.push_back({ x - 5.0, getWindowHeight()*0.5+20.0 });        
        indices.push_back(4); indices.push_back(5);

        indices.push_back(1); indices.push_back(5);

        std::shared_ptr<HudMesh> mesh(new HudMesh(vertexLayout, GL_LINES));
        //mesh->addVertices(std::move(vertices), std::move(indices));
        //mesh->compileVertexBuffer();
        Tangram::MeshData<LineVertex> md;
        md.indices = indices;
        md.vertices = vertices;
        md.offsets.emplace_back(indices.size(), vertices.size());
        
        std::cout << "zoom init d" << std::endl;
        
        mesh->compile(md);

        m_fixed = mesh;
    }
}

void SlideZoom::draw(Tangram::RenderState& rs, std::unique_ptr<Tangram::Map>& pMap){

    if ( slider > height ) {
        slider -= height;
    } else if ( slider < -height ) {
        slider += height;
    }

    m_trnShader->use(rs);
    Tangram::UniformLocation trn_u_offset("u_offset");
    Tangram::UniformLocation trn_u_mask("u_mask");
    Tangram::UniformLocation trn_u_resolution("u_resolution");
    Tangram::UniformLocation trn_u_modelViewProjectionMatrix("u_modelViewProjectionMatrix");
    
    Tangram::UniformLocation fix_u_mask("u_mask");
    Tangram::UniformLocation fix_u_modelViewProjectionMatrix("u_modelViewProjectionMatrix");

    m_trnShader->setUniformf(rs, trn_u_offset, x, slider);
    m_trnShader->setUniformf(rs, trn_u_mask, x, y, x+width, y+height);
    m_trnShader->setUniformf(rs, trn_u_resolution, getWindowWidth(),getWindowHeight());
    m_trnShader->setUniformMatrix4f(rs, trn_u_modelViewProjectionMatrix, pMap->getView().getOrthoViewportMatrix());
    m_verticalRulerMeshA->draw(rs, *(m_trnShader.get()));
    glLineWidth(2.0f);
    m_verticalRulerMeshB->draw(rs, *(m_trnShader.get()));
    glLineWidth(1.0f);

    m_trnShader->use(rs);
    
    //std::cout << "Zoom = " << zoom << std::endl;
                                                 //was 15.
    m_trnShader->setUniformf(rs, trn_u_offset, x-15., getWindowHeight()*0.5-(zoom-9)*2.1f) ;
    m_trnShader->setUniformf(rs, trn_u_mask, 0, 0, getWindowWidth(), getWindowHeight());
    m_trnShader->setUniformMatrix4f(rs, trn_u_modelViewProjectionMatrix, pMap->getView().getOrthoViewportMatrix());
    
    m_triangle->draw(rs, *(m_trnShader.get()));

    m_fixShader->use(rs);
        
    m_fixShader->setUniformf(rs, fix_u_mask, 0, 0, getWindowWidth(), getWindowHeight());
    m_fixShader->setUniformMatrix4f(rs, fix_u_modelViewProjectionMatrix, pMap->getView().getOrthoViewportMatrix());
    // glLineWidth(2.0f);
    m_fixed->draw(rs, *(m_fixShader.get()));
    // glLineWidth(1.0f);
}
