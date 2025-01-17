#include "rectangle.h"
#include "util/geom.h"
#include <algorithm>
//#include "utils.h"

Rectangle::Rectangle():x(0.0), y(0.0), width(0.0), height(0.0){

}

Rectangle::Rectangle(const glm::vec4 &_vec4){
    set(_vec4);
}

Rectangle::Rectangle(const glm::ivec4 &_vec4){
    set(_vec4);
}

Rectangle::Rectangle(const Rectangle &_rect, const float &_margin){
    set(_rect.x-_margin, _rect.y-_margin, _rect.width+_margin*2., _rect.height+_margin*2.);
}

Rectangle::Rectangle(const float &_x, const float &_y, const float &_width, const float &_height){
    set(_x, _y, _width, _height);
}

Rectangle::~Rectangle(){
}

void Rectangle::set(const glm::vec4 &_vec4){
    set(_vec4.x, _vec4.y, _vec4.z, _vec4.w);
}

void Rectangle::set(const glm::ivec4 &_vec4){
    set(_vec4.x, _vec4.y, _vec4.z, _vec4.w);
}

void Rectangle::set(const float &_x, const float &_y, const float &_width, const float &_height){
    x = _x;
    y = _y;
    width = _width;
    height = _height;
}

void Rectangle::translate(const glm::vec3 &_pos){
    x += _pos.x;
    y += _pos.y;
}

//----------------------------------------------------------
glm::vec3 Rectangle::getMin() const {
    return glm::vec3(getMinX(),getMinY(),0.);
}

//----------------------------------------------------------
glm::vec3 Rectangle::getMax() const {
    return glm::vec3(getMaxX(),getMaxY(),0.);
}

//----------------------------------------------------------
float Rectangle::getMinX() const {
    return std::min(x, x + width);  // - width
}

//----------------------------------------------------------
float Rectangle::getMaxX() const {
    return std::max(x, x + width);  // - width
}

//----------------------------------------------------------
float Rectangle::getMinY() const{
    return std::min(y, y + height);  // - height
}

//----------------------------------------------------------
float Rectangle::getMaxY() const {
    return std::max(y, y + height);  // - height
}

bool Rectangle::inside(const float &_px, const float &_py) const {
	return inside(glm::vec3(_px,_py,0.));
}

float Rectangle::getLeft() const {
    return getMinX();
}

//----------------------------------------------------------
float Rectangle::getRight() const {
    return getMaxX();
}

//----------------------------------------------------------
float Rectangle::getTop() const {
    return getMinY();
}

//----------------------------------------------------------
float Rectangle::getBottom() const {
    return getMaxY();
}

//----------------------------------------------------------
glm::vec3 Rectangle::getTopLeft() const {
    return getMin();
}

//----------------------------------------------------------
glm::vec3 Rectangle::getTopRight() const {
    return glm::vec3(getRight(),getTop(),0.);
}

//----------------------------------------------------------
glm::vec3 Rectangle::getBottomLeft() const {
    return glm::vec3(getLeft(),getBottom(),0.);
}

//----------------------------------------------------------
glm::vec3 Rectangle::getBottomRight() const {
    return getMax();
}

glm::vec3  Rectangle::getCenter() const {
	return glm::vec3(x + width * 0.5f, y + height * 0.5f, 0.0);
}

//----------------------------------------------------------
bool Rectangle::inside(const glm::vec3& p) const {
    return  p.x > getMinX() && p.y > getMinY() &&
            p.x < getMaxX() && p.y < getMaxY();
}

//----------------------------------------------------------
bool Rectangle::inside(const Rectangle& rect) const {
    return  inside(rect.getMinX(),rect.getMinY()) &&
            inside(rect.getMaxX(),rect.getMaxY());
}

//----------------------------------------------------------
bool Rectangle::inside(const glm::vec3& p0, const glm::vec3& p1) const {
    // check to see if a line segment is inside the rectangle
    return inside(p0) && inside(p1);
}

//----------------------------------------------------------
bool Rectangle::intersects(const Rectangle& rect) const {
    return (getMinX() < rect.getMaxX() && getMaxX() > rect.getMinX() &&
            getMinY() < rect.getMaxY() && getMaxY() > rect.getMinY());
}

void Rectangle::growToInclude(const glm::vec3& p){
    float x0 = std::min(getMinX(),p.x);
    float x1 = std::max(getMaxX(),p.x);
    float y0 = std::min(getMinY(),p.y);
    float y1 = std::max(getMaxY(),p.y);
    float w = x1 - x0;
    float h = y1 - y0;
    set(x0,y0,w,h);
}

void Rectangle::growToInclude(const std::vector<glm::vec3> &_points){
    for(auto &it: _points){
        growToInclude(it);
    }
}
