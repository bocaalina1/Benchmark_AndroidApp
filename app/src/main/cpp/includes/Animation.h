#ifndef ANIMATION_H
#define ANIMATION_H

#include <vector>
#include <map>
#include <string>
#include <functional>

#include "glm.hpp"

// Android Includes
#include <android/asset_manager.h>

// Assimp Includes
#include <assimp/scene.h>
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>

#include "Bone.h"
#include "model_animation.h"

namespace gps {

    class Model;

    struct AssimpNodeData
    {
        glm::mat4 transformation = glm::mat4(1.0f);
        std::string name;
        int childrenCount = 0;
        std::vector<AssimpNodeData> children;
    };

    class Animation
    {
    public:
        Animation() = default;

        // UPDATED: Added AAssetManager* to read the file from the APK
        Animation(const std::string& animationPath, Model* model, AAssetManager* assetManager);

        ~Animation();

        Bone* FindBone(const std::string& name);

        inline float getTicksPerSecond() { return m_TicksPerSecond; }
        inline float GetDuration() { return m_Duration; }
        inline const AssimpNodeData& GetRootNode() { return m_RootNode; }
        inline const std::map<std::string, BoneInfo>& GetBoneIDMap()
        {
            return m_BoneInfoMap;
        }
        inline int GetBoneCount() const { return (int)m_BoneInfoMap.size(); }

    private:
        void ReadMissingBones(const aiAnimation* animation, Model& model);
        void ReadHierarchyData(AssimpNodeData& dest, const aiNode* src);

    private:
        float m_Duration;
        int m_TicksPerSecond;
        std::vector<Bone> m_Bones;
        AssimpNodeData m_RootNode;
        std::map<std::string, BoneInfo> m_BoneInfoMap;
    };

}

#endif