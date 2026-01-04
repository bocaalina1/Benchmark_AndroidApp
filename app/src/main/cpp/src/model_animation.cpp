#include "../includes/model_animation.h"

// STB Image Implementation
//#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include <assimp/postprocess.h>
#include <iostream>
#include <sstream>
#include <fstream>
#include <cassert>

// Android Logging
#ifdef __ANDROID__
#include <android/log.h>
#define LOG_TAG "GPS_Model"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)
#else
#define LOGI(...) std::cout << __VA_ARGS__ << std::endl
#define LOGE(...) std::cerr << __VA_ARGS__ << std::endl
#endif

namespace gps
{
    // Define the static member
#ifdef __ANDROID__
    AAssetManager* Model::s_assetManager = nullptr;
#endif

    Model::Model(std::string const& path, bool gamma) : gammaCorrection(gamma)
    {
        loadModel(path);
    }

    void Model::Draw(Shader& shader)
    {
        for (unsigned int i = 0; i < meshes.size(); i++)
            meshes[i].Draw(shader);
    }

    void Model::loadModel(std::string const& path)
    {
#ifdef __ANDROID__
        if (!s_assetManager) {
            LOGE("CRITICAL: AssetManager not set! Call Model::SetAssetManager() first.");
            return;
        }

        AAsset* asset = AAssetManager_open(s_assetManager, path.c_str(), AASSET_MODE_BUFFER);
        if (!asset) {
            LOGE("Failed to open model asset: %s", path.c_str());
            return;
        }

        size_t size = AAsset_getLength(asset);
        std::vector<char> buffer(size);
        AAsset_read(asset, buffer.data(), size);
        AAsset_close(asset);

        // Load from memory
        // 'importer' is now a class member (see header) to keep m_Scene valid
        m_Scene = importer.ReadFileFromMemory(
                buffer.data(),
                size,
                aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_CalcTangentSpace | aiProcess_FlipUVs,
                path.c_str() // passing path as hint
        );
#else
        m_Scene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_CalcTangentSpace | aiProcess_FlipUVs);
#endif

        if (!m_Scene || m_Scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !m_Scene->mRootNode)
        {
            LOGE("ERROR::ASSIMP:: %s", importer.GetErrorString());
            return;
        }

        // Store directory for texture loading
        directory = path.substr(0, path.find_last_of('/'));

        LOGI("Model Loaded: %s", path.c_str());
        LOGI("  - Animations: %d", m_Scene->mNumAnimations);
        LOGI("  - Meshes: %d", m_Scene->mNumMeshes);

        processNode(m_Scene->mRootNode, m_Scene);
    }
    void Model::processNode(aiNode* node, const aiScene* scene)
    {
        // Process all the node's meshes
        for (unsigned int i = 0; i < node->mNumMeshes; i++)
        {
            aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
            meshes.push_back(processMesh(mesh, scene));
        }

        // Then do the same for each of its children
        for (unsigned int i = 0; i < node->mNumChildren; i++)
        {
            processNode(node->mChildren[i], scene);
        }
    }

    Mesh Model::processMesh(aiMesh* mesh, const aiScene* scene)
    {
        std::vector<Vertex> vertices;
        std::vector<unsigned int> indices;
        std::vector<Texture> textures;

        for (unsigned int i = 0; i < mesh->mNumVertices; i++)
        {
            Vertex vertex;
            SetVertexBoneDataToDefault(vertex);

            vertex.Position = AssimpGLMHelpers::GetGLMVec(mesh->mVertices[i]);
            vertex.Normal = AssimpGLMHelpers::GetGLMVec(mesh->mNormals[i]);

            if (mesh->mTextureCoords[0])
            {
                glm::vec2 vec;
                vec.x = mesh->mTextureCoords[0][i].x;
                vec.y = mesh->mTextureCoords[0][i].y;
                vertex.TexCoords = vec;
            }
            else
                vertex.TexCoords = glm::vec2(0.0f, 0.0f);

            vertices.push_back(vertex);
        }

        for (unsigned int i = 0; i < mesh->mNumFaces; i++)
        {
            aiFace face = mesh->mFaces[i];
            for (unsigned int j = 0; j < face.mNumIndices; j++)
                indices.push_back(face.mIndices[j]);
        }

        // Extract Bone Weights
        ExtractBoneWeightForVertices(vertices, mesh, scene);

        // Process Material
        if (mesh->mMaterialIndex >= 0)
        {
            aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];

            // 1. Try Loading Legacy Diffuse Maps
            std::vector<Texture> diffuseMaps = loadMaterialTextures(material, aiTextureType_DIFFUSE, "texture_diffuse", scene);
            textures.insert(textures.end(), diffuseMaps.begin(), diffuseMaps.end());

            // 2. NEW: Try Loading GLTF/GLB PBR Base Color
            // GLB files store the main texture here! We treat it as "texture_diffuse" for your shader.
            std::vector<Texture> pbrMaps = loadMaterialTextures(material, aiTextureType_BASE_COLOR, "texture_diffuse", scene);
            textures.insert(textures.end(), pbrMaps.begin(), pbrMaps.end());

            // 3. NEW: Try Loading Unknown (Sometimes Assimp puts embedded textures here)
            if (diffuseMaps.empty() && pbrMaps.empty()) {
                std::vector<Texture> unknownMaps = loadMaterialTextures(material, aiTextureType_UNKNOWN, "texture_diffuse", scene);
                textures.insert(textures.end(), unknownMaps.begin(), unknownMaps.end());
            }

            // 4. Specular maps
            std::vector<Texture> specularMaps = loadMaterialTextures(material, aiTextureType_SPECULAR, "texture_specular", scene);
            textures.insert(textures.end(), specularMaps.begin(), specularMaps.end());
        }
        return Mesh(vertices, indices, textures);
    }

    unsigned int Model::TextureFromFile(const char* path, const std::string& directory, bool gamma)
    {
        std::string filename = std::string(path);

        // Construct full path
        if (!directory.empty()) {
            filename = directory + "/" + filename;
        }

        unsigned int textureID;
        glGenTextures(1, &textureID);

        int width, height, nrComponents;
        unsigned char* data = nullptr;

#ifdef __ANDROID__
        if (s_assetManager) {
            AAsset* asset = AAssetManager_open(s_assetManager, filename.c_str(), AASSET_MODE_BUFFER);
            if (asset) {
                size_t size = AAsset_getLength(asset);
                std::vector<unsigned char> buffer(size);
                AAsset_read(asset, buffer.data(), size);
                AAsset_close(asset);

                // Decode from memory
                data = stbi_load_from_memory(buffer.data(), (int)size, &width, &height, &nrComponents, 0);
            } else {
                LOGE("Texture asset not found: %s", filename.c_str());
            }
        }
#else
        data = stbi_load(filename.c_str(), &width, &height, &nrComponents, 0);
#endif

        if (data)
        {
            GLenum format;
            if (nrComponents == 1)
#ifdef __ANDROID__
                format = GL_LUMINANCE;
#else
                format = GL_RED;
#endif
            else if (nrComponents == 3)
                format = GL_RGB;
            else if (nrComponents == 4)
                format = GL_RGBA;

            glBindTexture(GL_TEXTURE_2D, textureID);
            glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
            glGenerateMipmap(GL_TEXTURE_2D);

            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

            stbi_image_free(data);
        }
        else
        {
            LOGE("Texture failed to load at path: %s", filename.c_str());
            stbi_image_free(data);
        }

        return textureID;
    }

    unsigned int Model::loadEmbeddedTexture(const aiTexture* embeddedTexture)
    {
        unsigned int textureID;
        glGenTextures(1, &textureID);

        int width, height, nrComponents;
        unsigned char* data = nullptr;

        if (embeddedTexture->mHeight == 0)
        {
            data = stbi_load_from_memory(reinterpret_cast<unsigned char*>(embeddedTexture->pcData), embeddedTexture->mWidth, &width, &height, &nrComponents, 0);
        }
        else
        {
            width = embeddedTexture->mWidth;
            height = embeddedTexture->mHeight;
            nrComponents = 4; // Assimp embeds as ARGB8888 usually
            data = reinterpret_cast<unsigned char*>(embeddedTexture->pcData);
        }

        if (data)
        {
            GLenum format = (nrComponents == 4) ? GL_RGBA : GL_RGB;
            glBindTexture(GL_TEXTURE_2D, textureID);
            glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
            glGenerateMipmap(GL_TEXTURE_2D);

            // Set params
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

            if (embeddedTexture->mHeight == 0) stbi_image_free(data);
        }

        return textureID;
    }

    std::vector<Texture> Model::loadMaterialTextures(aiMaterial* mat, aiTextureType type, std::string typeName, const aiScene* scene)
    {
        std::vector<Texture> textures;
        for (unsigned int i = 0; i < mat->GetTextureCount(type); i++)
        {
            aiString str;
            mat->GetTexture(type, i, &str);

            // Check if texture was loaded before
            bool skip = false;
            for (unsigned int j = 0; j < textures_loaded.size(); j++)
            {
                if (std::strcmp(textures_loaded[j].path.data(), str.C_Str()) == 0)
                {
                    textures.push_back(textures_loaded[j]);
                    skip = true;
                    break;
                }
            }

            if (!skip)
            {
                Texture texture;
                std::string texPath = str.C_Str();

                // Check for embedded texture
                if (!texPath.empty() && texPath[0] == '*')
                {
                    int embeddedIndex = std::atoi(texPath.substr(1).c_str());
                    if (scene && embeddedIndex >= 0 && embeddedIndex < (int)scene->mNumTextures) {
                        texture.id = loadEmbeddedTexture(scene->mTextures[embeddedIndex]);
                    }
                }
                else
                {
                    texture.id = TextureFromFile(str.C_Str(), this->directory, false);
                }

                texture.type = typeName;
                texture.path = str.C_Str();
                textures.push_back(texture);
                textures_loaded.push_back(texture);
            }
        }
        return textures;
    }

   void Model::SetVertexBoneDataToDefault(Vertex& vertex)
    {
        for (int i = 0; i < MAX_BONE_INFLUENCE; i++)
        {
            vertex.BoneIDs[i] = -1;
            vertex.Weights[i] = 0.0f;
        }
    }

    void Model::SetVertexBoneData(Vertex& vertex, int boneID, float weight)
    {
        for (int i = 0; i < MAX_BONE_INFLUENCE; ++i)
        {
            if (vertex.BoneIDs[i] < 0)
            {
                vertex.Weights[i] = weight;
                vertex.BoneIDs[i] = boneID;
                break;
            }
        }
    }

    void Model::ExtractBoneWeightForVertices(std::vector<Vertex>& vertices, aiMesh* mesh, const aiScene* scene)
    {
        for (int boneIndex = 0; boneIndex < mesh->mNumBones; ++boneIndex)
        {
            int boneID = -1;
            std::string boneName = mesh->mBones[boneIndex]->mName.C_Str();

            if (m_BoneInfoMap.find(boneName) == m_BoneInfoMap.end())
            {
                BoneInfo newBoneInfo;
                newBoneInfo.id = m_BoneCounter;
                newBoneInfo.offset = AssimpGLMHelpers::ConvertMatrixToGLMFormat(mesh->mBones[boneIndex]->mOffsetMatrix);
                m_BoneInfoMap[boneName] = newBoneInfo;
                boneID = m_BoneCounter;
                m_BoneCounter++;
            }
            else
            {
                boneID = m_BoneInfoMap[boneName].id;
            }

            assert(boneID != -1);
            auto weights = mesh->mBones[boneIndex]->mWeights;
            int numWeights = mesh->mBones[boneIndex]->mNumWeights;

            for (int weightIndex = 0; weightIndex < numWeights; ++weightIndex)
            {
                int vertexId = weights[weightIndex].mVertexId;
                float weight = weights[weightIndex].mWeight;
                assert(vertexId <= vertices.size());
                SetVertexBoneData(vertices[vertexId], boneID, weight);
            }
        }
    }

#ifdef __ANDROID__
    unsigned int Model::createFallbackTexture() {
        unsigned int textureID;
        glGenTextures(1, &textureID);
        glBindTexture(GL_TEXTURE_2D, textureID);
        unsigned char whitePixel[] = { 255, 255, 255, 255 };
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, whitePixel);
        return textureID;
    }
#endif

}