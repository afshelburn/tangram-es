#pragma once

#include "glm/glm.hpp"
#include "gl/mesh.h"
#include "util/geom.h"

struct LineVertex {
    GLfloat x;
    GLfloat y;
};
typedef Tangram::Mesh<LineVertex> HudMesh;

inline std::shared_ptr<HudMesh> getCrossMesh(float width){
    
    std::shared_ptr<Tangram::VertexLayout> vertexLayout = std::shared_ptr<Tangram::VertexLayout>(new Tangram::VertexLayout({
        {"a_position", 2, GL_FLOAT, false, 0}
    }));
    std::vector<LineVertex> vertices;
    std::vector<short unsigned int> indices;

    vertices.push_back({ -width, 0.0f});
    vertices.push_back({ width, 0.0f});
    vertices.push_back({ 0.0f, -width});
    vertices.push_back({ 0.0f, width});
    
    indices.push_back(0); indices.push_back(1); 
    indices.push_back(2); indices.push_back(3);

    std::shared_ptr<HudMesh> mesh(new HudMesh(vertexLayout, GL_LINES));
    Tangram::MeshData<LineVertex> md;//(indices, vertices);
    md.indices = indices;
    md.vertices = vertices;
    md.offsets.emplace_back(indices.size(), vertices.size());
    mesh->compile(md);
    //mesh->addVertices(std::move(vertices), std::move(indices));
    //mesh->compileVertexBuffer();

    return mesh;
}

inline std::shared_ptr<HudMesh> getVerticalRulerMesh(float min, float max, float step, float width){
    
    std::shared_ptr<Tangram::VertexLayout> vertexLayout = std::shared_ptr<Tangram::VertexLayout>(new Tangram::VertexLayout({
        {"a_position", 2, GL_FLOAT, false, 0}
    }));
    std::vector<LineVertex> vertices;
    std::vector<short unsigned int> indices;

    float lenght = max-min;
    int nLines = lenght/step;

    for(int i = 0; i < nLines; i++){
        float y = min + (step*(float)i);
        vertices.push_back({0.0f, y});
        vertices.push_back({width, y});

        indices.push_back(i*2); indices.push_back(i*2+1); 
    }

    std::shared_ptr<HudMesh> mesh(new HudMesh(vertexLayout, GL_LINES));
    Tangram::MeshData<LineVertex> md;
    md.indices = indices;
    md.vertices = vertices;
    md.offsets.emplace_back(indices.size(), vertices.size());
    mesh->compile(md);
    //mesh->addVertices(std::move(vertices), std::move(indices));
    //mesh->compileVertexBuffer();

    return mesh;
}

inline std::shared_ptr<HudMesh> getCircularRulerMesh(float radius, int nLines, float width){
    
    std::shared_ptr<Tangram::VertexLayout> vertexLayout = std::shared_ptr<Tangram::VertexLayout>(new Tangram::VertexLayout({
        {"a_position", 2, GL_FLOAT, false, 0}
    }));
    std::vector<LineVertex> vertices;
    std::vector<short unsigned int> indices;

    float step = Tangram::TWO_PI/(float)nLines;
    float a = -Tangram::PI;
    for(int i = 0; i < nLines; i++){
        vertices.push_back({radius*(float)cos(a), 
                            radius*(float)sin(a)});
        vertices.push_back({(radius+width)*(float)cos(a), 
                            (radius+width)*(float)sin(a)});
        indices.push_back(i*2); indices.push_back(i*2+1);
        a += step;
    }

    std::shared_ptr<HudMesh> mesh(new HudMesh(vertexLayout, GL_LINES));
    Tangram::MeshData<LineVertex> md;
    md.indices = indices;
    md.vertices = vertices;
    md.offsets.emplace_back(indices.size(), vertices.size());
    mesh->compile(md);
    //mesh->addVertices(std::move(vertices), std::move(indices));
    //mesh->compileVertexBuffer();

    return mesh;
}

inline std::shared_ptr<HudMesh> getTriangle(const glm::vec2& pos, float width, float angle = 0){
    
    std::shared_ptr<Tangram::VertexLayout> vertexLayout = std::shared_ptr<Tangram::VertexLayout>(new Tangram::VertexLayout({
        {"a_position", 2, GL_FLOAT, false, 0}
    }));
    std::vector<LineVertex> vertices;
    std::vector<short unsigned int> indices;

    float step = Tangram::TWO_PI/3.0f;
    for(int i = 0; i < 3; i++){
        vertices.push_back({pos.x+width*(float)cos(angle), 
                            pos.y+width*(float)sin(angle)});
        indices.push_back(i);
        angle += step;
    }

    std::shared_ptr<HudMesh> mesh(new HudMesh(vertexLayout, GL_TRIANGLES));
    Tangram::MeshData<LineVertex> md;
    md.indices = indices;
    md.vertices = vertices;
    md.offsets.emplace_back(indices.size(), vertices.size());
    mesh->compile(md);
    //mesh->addVertices(std::move(vertices), std::move(indices));
    //mesh->compileVertexBuffer();

    return mesh;
}

inline std::shared_ptr<HudMesh> getTriangle(float width, float angle = 0){
    return getTriangle(glm::vec2(0.0,0.0),width,angle);
}
