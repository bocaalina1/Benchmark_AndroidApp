#version 300 es
precision highp float;

layout(location=0) in vec3 vPosition;
layout(location=1) in vec3 vNormal;
layout(location=2) in vec2 vTexCoords;
layout(location=3) in ivec4 vBoneIds;
layout(location=4) in vec4 vWeights;

out vec3 fNormal;
out vec4 fPosEye;
out vec2 fTexCoords;
out vec4 fragPosLightSpace;

const int MAX_BONES = 100;
const int MAX_BONE_INFLUENCE = 4;

uniform mat4 finalBonesMatrices[MAX_BONES];
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform mat3 normalMatrix;
uniform mat4 lightSpaceTrMatrix;

void main()
{
    vec4 totalPosition = vec4(0.0f);
    vec3 totalNormal = vec3(0.0f);
    float totalWeight = 0.0f;

    for(int i = 0 ; i < MAX_BONE_INFLUENCE ; i++)
    {
        int boneId = vBoneIds[i];
        float w = vWeights[i];

        if(boneId == -1 || w == 0.0f)
        continue;

        if(boneId >= MAX_BONES)
        {
            totalPosition = vec4(vPosition, 1.0f);
            totalNormal = vNormal;
            break;
        }

        vec4 localPosition = finalBonesMatrices[boneId] * vec4(vPosition, 1.0f);
        totalPosition += localPosition * w;

        vec3 localNormal = mat3(finalBonesMatrices[boneId]) * vNormal;
        totalNormal += localNormal * w;

        totalWeight += w;
    }

    if (totalWeight == 0.0f) {
        totalPosition = vec4(vPosition, 1.0f);
        totalNormal = vNormal;
    }

    fPosEye = view * model * totalPosition;
    fNormal = normalize(normalMatrix * totalNormal);
    fTexCoords = vTexCoords;
    fragPosLightSpace = lightSpaceTrMatrix * model * totalPosition;

    gl_Position = projection * fPosEye;
}