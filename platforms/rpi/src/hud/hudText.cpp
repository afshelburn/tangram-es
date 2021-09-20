#include "hudText.h"
#include "gl/renderState.h"
#include <iostream>
#include "context.h"
#include <cassert>

#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"

#include "gl/vertexLayout.h"
#include "util/geom.h"
#include <ft2build.h>
#include <freetype/freetype.h>
//#include "view/view.h"

//#include "map.h"

#include "wrap.h"

void HudText::init() {
	std::string frag =
"#ifdef GL_ES\n"
"precision mediump float;\n"
"#endif\n";

    // Fixed Vertex Shader
    std::string vertShader = frag;
    vertShader += STRINGIFY(
	attribute vec4 vPosition;
	varying vec2 texturePos;
	void main(void) {
	  gl_Position = vec4(vPosition.xy, 0.0, 1.0);
	  texturePos = vPosition.zw;
	});

	//void main(void) {
    //  vec4 sampled = vec4(1.0, 1.0, 1.0, texture2D(tex, texturePos).r);
	//  gl_FragColor = vec4 ( 1.0, 1.0, 1.0, 1.0 ) * sampled;
	//});
	
//vec4(1.0, 1.0, 1.0, texture2D(text, texturePos).r);
//uniform vec3 textColor;
    // Universal Fragment Shader
    frag += STRINGIFY(
	varying vec2 texturePos;
	uniform sampler2D tex;
	void main(void) {
	  gl_FragColor = vec4(texture2D(tex, texturePos).r, 0, 1, texture2D(tex, texturePos).r);
	});
	
    m_textShader = std::shared_ptr<Tangram::ShaderProgram>(new Tangram::ShaderProgram());
    m_textShader->setShaderSource(vertShader, frag);
    
}

GLuint myVAO;
GLuint myVBO;

void HudText::RenderText(Tangram::RenderState& rs, std::string t, float x, float y, float scale, glm::vec3 color)
{
    // activate corresponding render state	
    m_textShader->use(rs);
		glEnable(GL_TEXTURE_2D);
    	/*static GLfloat vVertices[] = {  -0.5f,  0.9f, 0.0f, 
                        -0.5f, 0.0f, 0.0f,
                         -0.25f, 0.0f, 0.0f,
                         -0.25f, 0.0f, 0.0f,
                         -0.25f, 0.9f, 0.0f,
                         -0.5f, 0.9f, 0.0f }; */
                            
        glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		
		GLuint tex_loc   = glGetUniformLocation( m_textShader->getGlProgram(), "tex" );
		std::cout << "tex_loc = " << tex_loc << std::endl;
        glUniform1i( tex_loc, 0 ); // 0, because the texture is bound to of texture unit 0

		assert(m_textShader->isValid());
		std::cout << "GLProgram = " << m_textShader->getGlProgram() << std::endl;
    
        glBindVertexArray(myVAO);
        glBindBuffer(GL_ARRAY_BUFFER, myVBO);
        //glBufferData(GL_ARRAY_BUFFER, sizeof(vVertices), vVertices, GL_STATIC_DRAW);
        glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0); // Info about where positions are in the VBO
        glEnableVertexAttribArray(0);    // Enable the stored vertex positions
    
        std::string::const_iterator c;
		for (c = t.begin(); c != t.end(); c++)
		{
			Character ch = Characters[*c];
			//Character ch = Characters[text.front()];
		
			float xpos = x + ch.Bearing.x * scale;
			float ypos = y - (ch.Size.y - ch.Bearing.y) * scale;

			float w = ch.Size.x * scale;
			float h = ch.Size.y * scale;
			// update VBO for each character
			float vertices[6][4] = {
				{ xpos,     ypos + h,   0.0f, 0.0f },            
				{ xpos,     ypos,       0.0f, 1.0f },
				{ xpos + w, ypos,       1.0f, 1.0f },

				{ xpos,     ypos + h,   0.0f, 0.0f },
				{ xpos + w, ypos,       1.0f, 1.0f },
				{ xpos + w, ypos + h,   1.0f, 0.0f }           
			};
			
			//std::cout << "Texture ID for " << text.front() << " is " << ch.TextureID << std::endl;
			//for(int i = 0; i < 6; i++) {
			//	std::cout << "vertex[" << i << "] = " << vertices[i][0] << ", " << vertices[i][1] << ", " << vertices[i][2] << ", " << vertices[i][3] << std::endl;
			//}
		
			glActiveTexture(GL_TEXTURE0);
			glBindVertexArray(myVAO);
			glEnableVertexAttribArray ( 0 );
			
			glBindTexture(GL_TEXTURE_2D, ch.TextureID);
			// update content of VBO memory
			glBindBuffer(GL_ARRAY_BUFFER, myVBO);
			glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices); 
			glBindBuffer(GL_ARRAY_BUFFER, 0);
			// render quad
			glBindTexture(GL_TEXTURE_2D, ch.TextureID);
			glEnable(GL_TEXTURE_2D);
			glDrawArrays(GL_TRIANGLES, 0, 6);
			
			x += (ch.Advance >> 6) * scale; // bitshift by 6 to get value in pixels (2^6 = 64)
		}
		
		assert(m_textShader->isValid());
		

}

void loadFace(std::map<char, Character>& Characters, FT_Face& face) {
	glActiveTexture(GL_TEXTURE0);
    glEnable(GL_TEXTURE_2D);
	for (unsigned char c = 0; c < 128; c++)
	{
		// load character glyph 
		if (FT_Load_Char(face, c, FT_LOAD_RENDER))
		{
			std::cout << "ERROR::FREETYTPE: Failed to load Glyph" << std::endl;
			continue;
		}
		// generate texture
		unsigned int texture;
		glGenTextures(1, &texture);
		glBindTexture(GL_TEXTURE_2D, texture);
		glPixelStorei(GL_UNPACK_ALIGNMENT, 1); // disable byte-alignment restriction
		glTexImage2D(
			GL_TEXTURE_2D,
			0,
			GL_LUMINANCE,
			face->glyph->bitmap.width,
			face->glyph->bitmap.rows,
			0,
			GL_LUMINANCE,
			GL_UNSIGNED_BYTE,
			face->glyph->bitmap.buffer
		);
		//std::cout << c << " data " << (int) face->glyph->bitmap.buffer << std::endl;
		//for(int j = 0; j < 30; j++) {
		//	std::cout << face->glyph->bitmap.width << " x " << face->glyph->bitmap.rows << std::endl;
		//	for(int w = 0; w < face->glyph->bitmap.width; w++) {
		//		for(int r = 0; r < face->glyph->bitmap.rows; r++) {
		//			//unsigned char** data = (unsigned char**) face->glyph->bitmap.buffer;
		//			//std::cout << data[w][r] << std::endl;
		//		}
		//	}
		//}
		// set texture options
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		// now store character for later use
		Character character = {
			texture, 
			glm::ivec2(face->glyph->bitmap.width, face->glyph->bitmap.rows),
			glm::ivec2(face->glyph->bitmap_left, face->glyph->bitmap_top),
			face->glyph->advance.x
		};
		Characters.insert(std::pair<char, Character>(c, character));
	}
}

void HudText::draw(Tangram::RenderState& rs) {
	
	std::cout << "Drawing text " << text << " with shader program " << m_textShader->getGlProgram() << std::endl;
	if(!m_textShader->isValid()) {
		std::cout << "Building shader" << std::endl;
		m_textShader->build(rs);
		assert(m_textShader->isValid());
		std::cout << "GLProgram = " << m_textShader->getGlProgram() << std::endl;
		glBindAttribLocation ( m_textShader->getGlProgram(), 0, "vPosition" );
		assert(m_textShader->isValid());
		std::cout << "GLProgram = " << m_textShader->getGlProgram() << std::endl;
		//glGenVertexArrays(1, &myVAO);
        //glGenBuffers(1, &myVBO);
		
		FT_Library ft;
		if (FT_Init_FreeType(&ft))
		{
			std::cout << "ERROR::FREETYPE: Could not init FreeType Library" << std::endl;
			//return -1;
		}

		FT_Face face;
		if (FT_New_Face(ft, "/usr/share/fonts/truetype/beteckna/BetecknaGSCondensed-Bold.ttf", 0, &face))
		{
			std::cout << "ERROR::FREETYPE: Failed to load font" << std::endl;  
			//return -1;
		}
		
		FT_Set_Pixel_Sizes(face, 0, 48);
		
		std::cout << "Loading font face from freetype" << std::endl;
		loadFace(Characters, face);
		
		FT_Done_Face(face);
		FT_Done_FreeType(ft);
		
		
		glGenVertexArrays(1, &myVAO);
		glGenBuffers(1, &myVBO);
		glBindVertexArray(myVAO);
		glBindBuffer(GL_ARRAY_BUFFER, myVBO);
		glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 6 * 4, NULL, GL_DYNAMIC_DRAW);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindVertexArray(0);  
		assert(m_textShader->isValid());
		std::cout << "Done loading" << std::endl;
	}
	
	assert(m_textShader->isValid());
	glViewport ( 0, 0, 800, 480 );
	glLineWidth(2.0f);
	glDisable(GL_CULL_FACE);
	RenderText(rs, "0123456789", -0.9f, -0.5f, 0.005f, glm::vec3(1.0f,0.0f,0.0f));
	assert(m_textShader->isValid());
	/*
	
	static GLfloat vVertices[] = {  -0.5f,  0.9f, 0.0f, 
                        -0.5f, 0.0f, 0.0f,
                         -0.25f, 0.0f, 0.0f,
                         -0.25f, 0.0f, 0.0f,
                         -0.25f, 0.9f, 0.0f,
                         -0.5f, 0.9f, 0.0f }; 
                            
        glBindVertexArray(myVAO);
        glBindBuffer(GL_ARRAY_BUFFER, myVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vVertices), vVertices, GL_STATIC_DRAW);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0); // Info about where positions are in the VBO
        glEnableVertexAttribArray(0);    // Enable the stored vertex positions
	
    //glEnableVertexArray(myVAO); 
       
	// Set the viewport
	std::cout << "viewport: " << getWindowWidth() << ", " << getWindowHeight() << std::endl;
	glViewport ( 0, 0, 800, 480 );

	// Clear the color buffer
	//glClear ( GL_COLOR_BUFFER_BIT );

	// Use the program object
	//glUseProgram ( userData->programObject );
	std::cout << "Drawing triangle" << std::endl;
	m_textShader->use(rs);

	// Load the vertex data
	//glBindAttribLocation ( m_textShader->getGlProgram(), 0, "vPosition" );
	//glVertexAttribPointer ( 0, 3, GL_FLOAT, GL_FALSE, 0, vVertices );
	glEnableVertexAttribArray ( 0 );
	glLineWidth(2.0f);
	glDisable(GL_CULL_FACE);
	glDrawArrays ( GL_TRIANGLES, 0, 6 );
	*/
}

