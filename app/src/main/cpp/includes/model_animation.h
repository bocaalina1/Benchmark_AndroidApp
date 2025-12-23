#ifndef MODEL_H
#define MODEL_H

#include "glm.hpp"
#include "gtc/matrix_transform.hpp"
#include <assimp/scene.h>
#include <assimp/Importer.hpp> // Required for Importer

#include "Mesh.hpp"
#include "Shader.hpp"
#include "AssimpGLMHelpers.h"
#include <string>
#include <map>
#include <vector>

// Android specific
#ifdef __ANDROID__
#include <android/asset_manager.h>
#endif

#define MAX_BONE_INFLUENCE 4

struct BoneInfo
{
    int id;
    glm::mat4 offset;
};

namespace gps
{
    class Model
    {
    public:
        // Model Data
        std::vector<Texture> textures_loaded;
        std::vector<Mesh> meshes;
        std::string directory;
        bool gammaCorrection;

#ifdef __ANDROID__
        // Static Asset Manager (Must be set before loading any model!)
        static AAssetManager* s_assetManager;
        static void SetAssetManager(AAssetManager* manager) { s_assetManager = manager; }
#endif

        // Constructor
        Model(std::string const& path, bool gamma = false);
        ~Model() = default;

        // Draw
        void Draw(Shader& shader);

        // Bone Accessors
        auto& GetBoneInfoMap() { return m_BoneInfoMap; }
        int& GetBoneCount() { return m_BoneCounter; }

    private:
        // Bone Data
        std::map<std::string, BoneInfo> m_BoneInfoMap;
        int m_BoneCounter = 0;

        // Assimp Scene
        // Note: We keep the importer as a member to ensure the scene pointer remains valid
        Assimp::Importer importer;
        const aiScene* m_Scene = nullptr;

        // Loading Functions
        void loadModel(std::string const& path);
        void processNode(aiNode* node, const aiScene* scene);
        Mesh processMesh(aiMesh* mesh, const aiScene* scene);

        // Bone Helpers
        void SetVertexBoneDataToDefault(Vertex& vertex);
        void SetVertexBoneData(Vertex& vertex, int boneID, float weight);
        void ExtractBoneWeightForVertices(std::vector<Vertex>& vertices, aiMesh* mesh, const aiScene* scene);

        // Texture Functions
        unsigned int TextureFromFile(const char* path, const std::string& directory, bool gamma);
        std::vector<Texture> loadMaterialTextures(aiMaterial* mat, aiTextureType type, std::string typeName, const aiScene* scene);
        unsigned int loadEmbeddedTexture(const aiTexture* embeddedTexture);

#ifdef __ANDROID__
        unsigned int createFallbackTexture();
#endif
    };
}

#endif // MODEL_H