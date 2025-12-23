#include "../includes/Mesh.hpp"
#include <cstddef> // Required for 'offsetof'
#include <GLES3/gl3.h> // Explicitly using GLES 3.0 headers

namespace gps {

    /* Mesh Constructor */
    Mesh::Mesh(std::vector<Vertex> vertices, std::vector<GLuint> indices, std::vector<Texture> textures) {

        this->vertices = vertices;
        this->indices = indices;
        this->textures = textures;

        this->setupMesh();
    }

    Buffers Mesh::getBuffers() {
        return this->buffers;
    }

    /* Mesh drawing function
       NOTE: Changed argument to 'gps::Shader&' (reference) to match Mesh.hpp
    */
    void Mesh::Draw(gps::Shader& shader) {
        shader.useShaderProgram();

        // 1. Bind appropriate textures
        unsigned int diffuseNr  = 1;
        unsigned int specularNr = 1;
        unsigned int normalNr   = 1;
        unsigned int heightNr   = 1;

        for(unsigned int i = 0; i < textures.size(); i++)
        {
            glActiveTexture(GL_TEXTURE0 + i); // Active proper texture unit before binding

            // Retrieve texture number (the N in diffuse_textureN)
            std::string number;
            std::string name = textures[i].type;

            if(name == "texture_diffuse")
                number = std::to_string(diffuseNr++);
            else if(name == "texture_specular")
                number = std::to_string(specularNr++);
            else if(name == "texture_normal")
                number = std::to_string(normalNr++); // transfer unsigned int to stream
            else if(name == "texture_height")
                number = std::to_string(heightNr++); // transfer unsigned int to stream

            // Now set the sampler to the correct texture unit
            // This connects "uniform sampler2D texture_diffuse1" to GL_TEXTURE0
            glUniform1i(glGetUniformLocation(shader.shaderProgram, (name + number).c_str()), i);

            // Bind the texture
            glBindTexture(GL_TEXTURE_2D, textures[i].id);
        }

        // 2. Draw Mesh
        glBindVertexArray(this->buffers.VAO);
        glDrawElements(GL_TRIANGLES, (GLsizei)this->indices.size(), GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);

        // 3. Reset to default
        glActiveTexture(GL_TEXTURE0);
    }
    // Initializes all the buffer objects/arrays
    void Mesh::setupMesh() {

        // Create buffers/arrays
        glGenVertexArrays(1, &this->buffers.VAO);
        glGenBuffers(1, &this->buffers.VBO);
        glGenBuffers(1, &this->buffers.EBO);

        glBindVertexArray(this->buffers.VAO);

        // Load data into vertex buffers
        glBindBuffer(GL_ARRAY_BUFFER, this->buffers.VBO);
        glBufferData(GL_ARRAY_BUFFER, this->vertices.size() * sizeof(Vertex), &this->vertices[0], GL_STATIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, this->buffers.EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, this->indices.size() * sizeof(GLuint), &this->indices[0], GL_STATIC_DRAW);

        // Set the vertex attribute pointers

        // 1. Vertex Positions (Location 0)
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid*)0);

        // 2. Vertex Normals (Location 1)
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid*)offsetof(Vertex, Normal));

        // 3. Vertex Texture Coords (Location 2)
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid*)offsetof(Vertex, TexCoords));

        // 4. BoneIDs (Location 3)
        // CRITICAL FOR ANIMATION: usage of glVertexAttribIPointer is correct for integers (BoneIDs).
        // This function expects the shader input to be 'layout(location=3) in ivec4 ...'
        glEnableVertexAttribArray(3);
        glVertexAttribIPointer(3, 4, GL_INT, sizeof(Vertex), (GLvoid*)offsetof(Vertex, BoneIDs));

        // 5. Weights (Location 4)
        glEnableVertexAttribArray(4);
        glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid*)offsetof(Vertex, Weights));

        glBindVertexArray(0);
    }
}