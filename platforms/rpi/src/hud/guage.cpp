#include "guage.h"

#include <iostream>
#include "context.h"

#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"

#include "gl/vertexLayout.h"
#include "util/geom.h"

#include "gl/renderState.h"
#include "view/view.h"

#include "map.h"

void Guage::init(){

    std::cout << "guage init a" << std::endl;
    
    value = 0;

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

    std::cout << "guage init b" << std::endl;

    // Universal Fragment Shader
    frag += STRINGIFY(
    uniform vec2 u_resolution;
    uniform vec4 u_mask;
    uniform vec4 u_color;
    varying float v_alpha;
    void main(void) {
        vec2 st = gl_FragCoord.xy;
        vec2 mask = step(u_mask.xy,st)*(1.0-step(u_mask.zw,st));
        gl_FragColor = vec4(u_color.xyz,v_alpha)*(mask.x*mask.y);
    });

    std::cout << "guage init b1" << std::endl;
    m_fixShader = std::shared_ptr<Tangram::ShaderProgram>(new Tangram::ShaderProgram());
    std::cout << "guage init b2" << std::endl;
    m_fixShader->setShaderSource(trnVertShader, frag);
    
    std::cout << "guage init b0" << std::endl;

    std::cout << "guage init b1" << std::endl;
    
    //m_verticalRulerMesh = getVerticalRulerMesh(-getWindowHeight(),getWindowHeight(),5.0f,getWindowWidth()*0.0131125);
    m_verticalMinorRulerMesh = getVerticalRulerMesh(-100,125,25.0f,getWindowWidth()*0.0194525);
	m_verticalMajorRulerMesh = getVerticalRulerMesh(-100,150,50.0f,getWindowWidth()*0.0294525);

    m_triangle = getTriangle(getWindowHeight()*0.05);

    std::cout << "guage init c" << std::endl;

}

void Guage::draw(Tangram::RenderState& rs, std::unique_ptr<Tangram::Map>& pMap){
   
    Tangram::UniformLocation fix_u_mask("u_mask");
    Tangram::UniformLocation fix_u_modelViewProjectionMatrix("u_modelViewProjectionMatrix");

    Tangram::UniformLocation trn_u_offset("u_offset");
    Tangram::UniformLocation trn_u_mask("u_mask");
    Tangram::UniformLocation trn_u_color("u_color");
    Tangram::UniformLocation trn_u_resolution("u_resolution");
    Tangram::UniformLocation trn_u_modelViewProjectionMatrix("u_modelViewProjectionMatrix");
    
    m_fixShader->use(rs);
    
    float dy = (0.5-value)*200.0f;
    m_fixShader->setUniformf(rs, trn_u_offset, x-25, getWindowHeight()*0.5+dy);
    m_fixShader->setUniformf(rs, trn_u_mask, x-100, y-1000, x+width, y+height+1000);
    m_fixShader->setUniformf(rs, trn_u_resolution, getWindowWidth(),getWindowHeight());
    m_fixShader->setUniformMatrix4f(rs, trn_u_modelViewProjectionMatrix, pMap->getView().getOrthoViewportMatrix());
    
    //glPushAttrib(GL_LINE_BIT);
    
    if(value < 0.125f) {
		m_fixShader->setUniformf(rs, trn_u_color, 1.0f, 0.0f, 0.0f, 1.0f);
    }else if(value < 0.25f) {
		m_fixShader->setUniformf(rs, trn_u_color, 1.0f, 1.0f, 0.0f, 1.0f);
	}else{
		m_fixShader->setUniformf(rs, trn_u_color, 0.0f, 1.0f, 0.0f, 1.0f);
	}
    glLineWidth(2.0f);
    m_triangle->draw(rs, *(m_fixShader.get()));

    m_fixShader->setUniformf(rs, trn_u_offset, x, getWindowHeight()*0.5);
    m_fixShader->setUniformf(rs, trn_u_color, 0.0f, 0.0f, 1.0f, 1.0f);
    //m_fixShader->setUniformf(rs, fix_u_mask, 0, 0, getWindowWidth(), getWindowHeight());
    //m_fixShader->setUniformMatrix4f(rs, fix_u_modelViewProjectionMatrix, pMap->getView().getOrthoViewportMatrix());
    glLineWidth(2.5f);
    m_verticalMinorRulerMesh->draw(rs, *(m_fixShader.get()));
    glLineWidth(5.5f);
    m_verticalMajorRulerMesh->draw(rs, *(m_fixShader.get()));
    glLineWidth(1.0f);
    //glPopAttrib();

}

