#include "../includes/Animator.h" // Ensure this path matches your project structure
#include <android/log.h>
#include <cmath> // For fmod

#define LOG_TAG "GPS_Animator"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

namespace gps {

    Animator::Animator(Animation* animation)
    {
        m_CurrentTime = 0.0f;
        m_DeltaTime = 0.0f;
        m_CurrentAnimation = animation;

        if (animation) {
            int boneCount = animation->GetBoneCount();

            // Initialize with identity matrices
            m_FinalBoneMatrices.resize(boneCount, glm::mat4(1.0f));

            LOGI("Animator initialized with %d bones", boneCount);
        }
    }

    void Animator::UpdateAnimation(float dt)
    {
        m_DeltaTime = dt;
        if (m_CurrentAnimation)
        {
            // Advance time
            m_CurrentTime += m_CurrentAnimation->getTicksPerSecond() * dt;

            // Loop animation
            // fmod keeps the time within the duration [0, duration]
            m_CurrentTime = fmod(m_CurrentTime, m_CurrentAnimation->GetDuration());

            // Traverse the hierarchy to update matrices
            CalculateBoneTransform(&m_CurrentAnimation->GetRootNode(), glm::mat4(1.0f));
        }
    }

    void Animator::PlayAnimation(Animation* pAnimation)
    {
        if (!pAnimation) return;

        m_CurrentAnimation = pAnimation;
        m_CurrentTime = 0.0f;

        // Ensure vector is large enough for the new animation
        int boneCount = pAnimation->GetBoneCount();
        if (m_FinalBoneMatrices.size() < boneCount) {
            m_FinalBoneMatrices.resize(boneCount, glm::mat4(1.0f));
            LOGI("Animator resized buffer for new animation: %d bones", boneCount);
        }
    }

    void Animator::CalculateBoneTransform(const AssimpNodeData* node, glm::mat4 parentTransform)
    {
        std::string nodeName = node->name;
        glm::mat4 nodeTransform = node->transformation;

        // 1. Check if this node is a bone and needs interpolation
        Bone* bone = m_CurrentAnimation->FindBone(nodeName);

        if (bone)
        {
            bone->Update(m_CurrentTime);
            nodeTransform = bone->GetLocalTransform();
        }

        // 2. Calculate global transform relative to parent
        glm::mat4 globalTransformation = parentTransform * nodeTransform;

        // 3. If this is a skinned bone, store the final matrix
        auto boneInfoMap = m_CurrentAnimation->GetBoneIDMap();
        if (boneInfoMap.find(nodeName) != boneInfoMap.end())
        {
            int index = boneInfoMap[nodeName].id;
            glm::mat4 offset = boneInfoMap[nodeName].offset;

            // ANDROID SAFETY CHECK: Prevent Array Index Out of Bounds Crash
            if (index < m_FinalBoneMatrices.size()) {
                m_FinalBoneMatrices[index] = globalTransformation * offset;
            } else {
                LOGE("Error: Bone index %d out of bounds (Size: %d)", index, (int)m_FinalBoneMatrices.size());
            }
        }

        // 4. Recursively update children
        for (unsigned int i = 0; i < node->childrenCount; i++)
            CalculateBoneTransform(&node->children[i], globalTransformation);
    }
}