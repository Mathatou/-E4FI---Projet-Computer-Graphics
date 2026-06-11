#include "Model.hpp"
#include "libs/stb-master/tiny_obj_loader.h"


Model::Model() : VAO(0), VBO(0), IBO(0), indexCount(0), indexType(GL_UNSIGNED_INT) {}

Model::~Model() {
    if (VBO) glDeleteBuffers(1, &VBO);
    if (IBO) glDeleteBuffers(1, &IBO);
    if (VAO) glDeleteVertexArrays(1, &VAO);
}

bool Model::Load(const std::string& filename)
{
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warn, err;
    
    std::string basedir = "";
    size_t pos = filename.find_last_of("/");
    if (pos != std::string::npos) {
        basedir = filename.substr(0, pos + 1);
    }


    bool ret = tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, filename.c_str(), basedir.c_str());
    if (!warn.empty()) {
        std::cout << "TinyObj WARNING: " << warn << std::endl;
    }
    if (!err.empty()) {
        std::cerr << "TinyObj ERROR: " << err << std::endl;
        return false;
    }
    if(!ret){
        std::cerr << "Failed to load/parse .obj file." << std::endl;
        return false;
    }
    if(!materials.empty()){
        // On prend le premier matériau trouvé (s'il y en a)
        const auto& mat = materials[0];
        for (int i = 0; i < 3; i++) {
            material.ambient[i] = mat.ambient[i];
            material.diffuse[i] = mat.diffuse[i];
            material.specular[i] = mat.specular[i];
        }
        material.shininess = mat.shininess;
    }

    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;

    for(size_t s = 0; s < shapes.size(); s++) 
    {
        size_t index_offset = 0;
        for(size_t f = 0; f < shapes[s].mesh.num_face_vertices.size(); f++) 
        {
            for(size_t v = 0; v < 3; v++) 
            { // On suppose que le mesh est triangulé
                tinyobj::index_t idx = shapes[s].mesh.indices[index_offset + v];
                Vertex vertex;

                // Position
                vertex.position[0] = attrib.vertices[3 * idx.vertex_index + 0];
                vertex.position[1] = attrib.vertices[3 * idx.vertex_index + 1];
                vertex.position[2] = attrib.vertices[3 * idx.vertex_index + 2];

                // Normale
                if (idx.normal_index >= 0) {
                    vertex.normal[0] = attrib.normals[3 * idx.normal_index + 0];
                    vertex.normal[1] = attrib.normals[3 * idx.normal_index + 1];
                    vertex.normal[2] = attrib.normals[3 * idx.normal_index + 2];
                } else {
                    vertex.normal[0] = vertex.normal[2] = 0.0f;
                    vertex.normal[1] = 1.0f; // Normale par défaut pointant vers le haut
                }

                // Texture
                if (idx.texcoord_index >= 0) {
                    vertex.texCoord[0] = attrib.texcoords[2 * idx.texcoord_index + 0];
                    vertex.texCoord[1] = attrib.texcoords[2 * idx.texcoord_index + 1];
                } else {
                    vertex.texCoord[0] = vertex.texCoord[1] = 0.0f;
                }

                vertices.push_back(vertex);
                indices.push_back(static_cast<unsigned int>(indices.size()));
            }
            index_offset += 3;
        }
        
    }
    indexCount = static_cast<GLsizei>(indices.size());
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &IBO);

    glBindVertexArray(VAO);
    
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), vertices.data(), GL_STATIC_DRAW);
    
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, IBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

    // Position
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, position));
    // Normale 
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));
    // Texture
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, texCoord));
    
    glBindVertexArray(0);
    indexType = GL_UNSIGNED_INT;
    return true;
}

bool Model::LoadFromData(const float* vertexData, size_t vertexCount, const uint16_t* indexData, size_t indexCount)
{
    this->indexCount=static_cast<GLsizei>(indexCount);
    this->indexType = GL_UNSIGNED_SHORT;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &IBO);
    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertexCount * sizeof(float), vertexData, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, IBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indexCount * sizeof(uint16_t), indexData, GL_STATIC_DRAW);

    // Position
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 8, (void*)0);
    // Normale
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 8, (void*)(sizeof(float) * 3));
    // Texture
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 8, (void*)(sizeof(float) * 6));

    glBindVertexArray(0);
    return true;
}


void Model::Draw() const
{
    if(VAO == 0) return;
    glBindVertexArray(VAO);
    glDrawElements(GL_TRIANGLES, indexCount, indexType, (void*)0);
    glBindVertexArray(0);
}


