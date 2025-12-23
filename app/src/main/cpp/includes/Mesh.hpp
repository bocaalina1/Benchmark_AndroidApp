#ifndef Mesh_hpp
#define Mesh_hpp

#include <GLES3/gl3.h>

#include "glm.hpp"
#include "Shader.hpp"

#include <string>
#include <vector>

namespace gps {

    struct Vertex {
        glm::vec3 Position;
        glm::vec3 Normal;
        glm::vec2 TexCoords;

        // For skeletal animation
        glm::ivec4 BoneIDs;
        glm::vec4 Weights;
    };

    struct Texture {
        GLuint id;
        std::string type;
        std::string path;
    };

    struct Material {
        glm::vec3 ambient;
        glm::vec3 diffuse;
        glm::vec3 specular;
    };

    struct Buffers {
        GLuint VAO;
        GLuint VBO;
        GLuint EBO;
    };

    class Mesh {

    public:
        std::vector<Vertex> vertices;
        std::vector<GLuint> indices;
        std::vector<Texture> textures;

        Mesh(std::vector<Vertex> vertices, std::vector<GLuint> indices, std::vector<Texture> textures);

        Buffers getBuffers();

        void Draw(gps::Shader& shader);

    private:
        /* Render data  */
        Buffers buffers;

        // Initializes all the buffer objects/arrays
        void setupMesh();

    };

}
#endif /* Mesh_hpp */