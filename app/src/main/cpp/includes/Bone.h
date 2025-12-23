#ifndef BONE_H
#define BONE_H

#pragma once

#include <vector>
#include <string>
#include <list>
#include <cassert> // For assert()

#include <assimp/scene.h> // For aiNodeAnim, aiVector3D, aiQuaternion
#include "glm.hpp"
#define GLM_ENABLE_EXPERIMENTAL
#include "gtx/quaternion.hpp"
#include "gtx/transform.hpp" // For glm::translate, glm::scale
#include "AssimpGLMHelpers.h"

struct KeyPosition
{
	glm::vec3 position;
	float timeStamp;
};

struct KeyRotation
{
	glm::quat orientation;
	float timeStamp;
};

struct KeyScale
{
	glm::vec3 scale;
	float timeStamp;
};

class Bone
{
public:
	// Constructor: Loads keyframes from an Assimp animation channel
	Bone(const std::string& name, int ID, const aiNodeAnim* channel);

	// Calculates the interpolated transform for a given time
	void Update(float animationTime);

	// Accessors
	glm::mat4 GetLocalTransform() { return m_LocalTransform; }
	std::string GetBoneName() const{ return m_Name; }
	int GetBoneID() { return m_ID; }

private:
	// Private Helper Functions (Prototypes)

	// Finds the index of the keyframe *before* the current animationTime
	int GetPositionIndex(float animationTime);
	int GetRotationIndex(float animationTime);
	int GetScaleIndex(float animationTime);

	// Calculates the blending factor (0.0 to 1.0) between two keyframes
	float GetScaleFactor(float lastTimeStamp, float nextTimeStamp, float animationTime);

	// Interpolation Functions: Calculate and convert to mat4
	glm::mat4 InterpolatePosition(float animationTime);
	glm::mat4 InterpolateRotation(float animationTime);
	glm::mat4 InterpolateScaling(float animationTime);

private:
	// Private Member Variables
	std::vector<KeyPosition> m_Positions;
	std::vector<KeyRotation> m_Rotations;
	std::vector<KeyScale> m_Scales;

	int m_NumPositions;
	int m_NumRotations;
	int m_NumScalings;

	glm::mat4 m_LocalTransform; // The current interpolated transform
	std::string m_Name;
	int m_ID;
};

#endif // BONE_H