#ifndef Animator_hpp
#define Animator_hpp

#include "glm.hpp"
#include <map>
#include <vector>
#include <assimp/scene.h>
#include <assimp/Importer.hpp>

#include "Animation.h"
#include "Bone.h"

namespace gps {
    class Animator
    {
    public:
        // Constructor
        Animator(Animation* animation);

        // Call this every frame with the delta time (in seconds)
        void UpdateAnimation(float dt);

        // Switch to a new animation
        void PlayAnimation(Animation* pAnimation);

        // Recursive function to update bone matrices
        void CalculateBoneTransform(const AssimpNodeData* node, glm::mat4 parentTransform);

        // OPTIMIZATION: Return by const reference (&) to avoid copying vector every frame
        const std::vector<glm::mat4>& GetFinalBoneMatrices() { return m_FinalBoneMatrices; }

    private:
        std::vector<glm::mat4> m_FinalBoneMatrices;
        Animation* m_CurrentAnimation;
        float m_CurrentTime;
        float m_DeltaTime;
    };
}
#endif