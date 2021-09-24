#pragma once

#include "gl/shaderProgram.h"
#include <iostream>
#include <map>
#include <string>
#include "rectangle.h"

namespace Tangram {
	class RenderState;
}

struct Character {
    unsigned int TextureID;  // ID handle of the glyph texture
    glm::ivec2   Size;       // Size of glyph
    glm::ivec2   Bearing;    // Offset from baseline to left/top of glyph
    unsigned int Advance;    // Offset to advance to next glyph
};

class HudText : public Rectangle {
public:

	void init();
	
	void draw(Tangram::RenderState& rs);
	
	void setFont(std::string font, int size) {
		this->font = font;
		this->size = size;
	}
	
	void setText(std::string text, glm::vec2 pos, float scale) {
		this->text = text;
		this->pos = pos;
		this->scale = scale;
	}
	
	void setText(std::string text) {
		this->text = text;
	}
	
	void RenderText(Tangram::RenderState& rs, std::string text, float x, float y, float scale, glm::vec3 color);
	
protected:
	std::string font;
	std::string text;
	int size;
	glm::vec2 pos;
	float scale;
	
	std::shared_ptr<Tangram::ShaderProgram>  m_textShader;
	std::map<char, Character> Characters;
	
	//"/usr/share/fonts/truetype/beteckna/BetecknaGSCondensed-Bold.ttf"
};
