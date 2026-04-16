#ifndef MESH_H
#define MESH_H

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <unordered_map>
#include <vector>

// holds all vertex data
struct Vertex {
    glm::vec3 Position;
    glm::vec3 Normal;
    glm::vec2 TexCoords;
};

class Mesh {
public:
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    unsigned int textureID;

    // creates the buffers
    Mesh(std::vector<Vertex> &vertices, std::vector<unsigned int> &indices, unsigned int textureID) {
        this->vertices = vertices;
        this->indices = indices;
        this->textureID = textureID;
        setupMesh();
    }

    // call to draw the mesh with textures
    void draw(unsigned int shaderProgram) {
        struct TextureUniformLocations {
            int hasTexture = -1;
            int mytexture = -1;
        };

        static std::unordered_map<unsigned int, TextureUniformLocations> uniformCache;
        TextureUniformLocations& locations = uniformCache[shaderProgram];
        if (locations.hasTexture == -1 || locations.mytexture == -1) {
            locations.hasTexture = glGetUniformLocation(shaderProgram, "hasTexture");
            locations.mytexture = glGetUniformLocation(shaderProgram, "mytexture");
        }

        // bind texture if one exists (0 means no texture)
        if (textureID != 0) {
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, textureID);
            glUniform1i(locations.hasTexture, 1);
            glUniform1i(locations.mytexture, 0); 
        }
        else{
            glUniform1i(locations.hasTexture, 0); 
        }

        // draw mesh
        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    void cleanup() {
        glDeleteVertexArrays(1, &VAO);
        glDeleteBuffers(1, &VBO);
        glDeleteBuffers(1, &EBO);
    }

private:
    unsigned int VAO, VBO, EBO;

    void setupMesh() {
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
        glGenBuffers(1, &EBO);

        glBindVertexArray(VAO);

        // load data into vertex buffers
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), &vertices[0], GL_STATIC_DRAW); 

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &indices[0], GL_STATIC_DRAW);

        // set the vertex attribute pointers
        // Vertex Positions (Location = 0 in vertex shader)
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);

        // Vertex Normals (Location = 1 in vertex shader)
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Normal));

        // Vertex Texture Coords (Location = 2 in vertex shader)
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, TexCoords));

        glBindVertexArray(0);
    }
};

#endif