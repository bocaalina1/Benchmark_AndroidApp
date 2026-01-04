#include "../includes/Animation.h"
#include "../includes/model_animation.h" // Ensure Model is included
#include <android/log.h>
#include <android/asset_manager.h>
#include <vector>
#include <iostream>

// Define Logging Macros
#define LOG_TAG "GPS_Animation"
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

namespace gps {
    Animation::Animation(const std::string& animationPath, Model* model, AAssetManager* assetManager)
    {
        Assimp::Importer importer;
        const aiScene* scene = nullptr;
        AAsset* asset = AAssetManager_open(assetManager, animationPath.c_str(), AASSET_MODE_BUFFER);
        if (!asset) {
            LOGE("ERROR: Could not open animation file: %s", animationPath.c_str());
            return;
        }

        off_t length = AAsset_getLength(asset);
        std::vector<char> buffer(length);
        AAsset_read(asset, buffer.data(), length);
        AAsset_close(asset);

        scene = importer.ReadFileFromMemory(buffer.data(), length, aiProcess_Triangulate);

        if (!scene || !scene->mRootNode) {
            LOGE("ERROR: Assimp failed to load animation: %s", importer.GetErrorString());
            return;
        }

        if (scene->mNumAnimations == 0) {
            LOGE("ERROR: No animations found in file: %s", animationPath.c_str());
            return;
        }

        auto animation = scene->mAnimations[0];
        m_Duration = animation->mDuration;
        m_TicksPerSecond = animation->mTicksPerSecond;

       aiMatrix4x4 globalTransformation = scene->mRootNode->mTransformation;
        globalTransformation = globalTransformation.Inverse();

        ReadHierarchyData(m_RootNode, scene->mRootNode);
        ReadMissingBones(animation, *model);

        LOGI("Animation loaded successfully: %s", animationPath.c_str());
    }

    Animation::~Animation() {}

    Bone* Animation::FindBone(const std::string& name)
    {
        auto iter = std::find_if(m_Bones.begin(), m_Bones.end(),
                                 [&](const Bone& bone)
                                 {
                                     return bone.GetBoneName() == name;
                                 }
        );
        if (iter == m_Bones.end()) return nullptr;
        else return &(*iter);
    }

    void Animation::ReadMissingBones(const aiAnimation* animation, Model& model)
    {
        int size = animation->mNumChannels;
        auto& boneInfoMap = model.GetBoneInfoMap(); // Ensure this returns std::map<string, BoneInfo>&

        // CAUTION: Ensure model.GetBoneCount() returns a REFERENCE (int&)
        // If it returns 'int', this line will fail or not update the model counter.
        int& boneCount = model.GetBoneCount();

        for (int i = 0; i < size; i++)
        {
            auto channel = animation->mChannels[i];
            std::string boneName = channel->mNodeName.data;

            if (boneInfoMap.find(boneName) == boneInfoMap.end())
            {
                boneInfoMap[boneName].id = boneCount;
                boneCount++;
            }
            m_Bones.push_back(Bone(channel->mNodeName.data,
                                   boneInfoMap[channel->mNodeName.data].id, channel));
        }
        m_BoneInfoMap = boneInfoMap;
    }

    void Animation::ReadHierarchyData(AssimpNodeData& dest, const aiNode* src)
    {
        // assert(src); // Optional: assert crashes app hard, logging is often better on mobile
        if (!src) return;

        dest.name = src->mName.data;

        // Ensure AssimpGLMHelpers is defined in your project (usually in model_animation.h)
        dest.transformation = AssimpGLMHelpers::ConvertMatrixToGLMFormat(src->mTransformation);
        dest.childrenCount = src->mNumChildren;

        for (int i = 0; i < src->mNumChildren; i++)
        {
            AssimpNodeData newData;
            ReadHierarchyData(newData, src->mChildren[i]);
            dest.children.push_back(newData);
        }
    }

} // namespace gps