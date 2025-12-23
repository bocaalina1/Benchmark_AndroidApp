#version 300 es
precision highp float;

layout(location=0) in vec3 vPosition;
layout(location=3) in ivec4 vBoneIds;
layout(location=4) in vec4 vWeights;

const int MAX_BONES = 100;
const int MAX_BONE_INFLUENCE = 4;

uniform mat4 finalBonesMatrices[MAX_BONES];
uniform mat4 model;
uniform mat4 lightSpaceTrMatrix;

void main() {
    vec4 skinnedPos = vec4(0.0);
    float totalWeight = 0.0;
    
    for (int i = 0; i < MAX_BONE_INFLUENCE; i++) {
        int boneId = vBoneIds[i];
        float w = vWeights[i];
        
        if (boneId < 0 || w == 0.0)
            continue;
        
        skinnedPos += (finalBonesMatrices[boneId] * vec4(vPosition, 1.0)) * w;
        totalWeight += w;
    }
    
    if (totalWeight == 0.0) {
        skinnedPos = vec4(vPosition, 1.0);
    }
    
    gl_Position = lightSpaceTrMatrix * model * skinnedPos;
}