#pragma once
#include <GL/glew.h>
#include <string>
#include <vector>
#include <iostream>
#include <cstdint> 


struct Vertex {
    float position[3];
    float normal[3];
    float texCoord[2];
};


struct Material {
    float ambient[3] = {0.2f, 0.2f, 0.2f};
    float diffuse[3] = {0.8f, 0.8f, 0.8f};
    float specular[3] = {1.0f, 1.0f, 1.0f};
    float shininess = 10.0f;

    bool hasTexture = false;   
    GLuint diffuseTexture = 0;
};

class Model
{
    public:
        Model();
        ~Model();

        bool Load(const std::string& filename);
        bool LoadFromData(const float* vertexData, size_t vertexCount, const uint16_t* indexData, size_t indexCount);        
        void Draw() const;

        Material material;

    private:
        GLuint VAO, VBO, IBO;
        GLsizei indexCount;
        GLenum indexType;
};