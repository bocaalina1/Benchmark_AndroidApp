#version 300 es
precision highp float;

layout(location=0) in vec3 vPosition;
layout(location=1) in vec3 vNormal;
layout(location=2) in vec2 vTexCoords;

out vec3 fNormal;
out vec4 fPosEye;
out vec2 fTexCoords;
out vec4 fragPosLightSpace;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform mat3 normalMatrix;
uniform mat4 lightSpaceTrMatrix;

void main()
{
	// 1. Compute Eye Space Position (Used for Fog/Lighting)
	fPosEye = view * model * vec4(vPosition, 1.0f);

	// 2. Pass Normals and TexCoords
	fNormal = normalize(normalMatrix * vNormal);
	fTexCoords = vTexCoords;

	// 3. Compute Clip Space Position (Required by GPU)
	// Optimization: Reuse fPosEye instead of multiplying matrices again
	gl_Position = projection * fPosEye;

	// 4. Compute Shadow Map Coordinates
	fragPosLightSpace = lightSpaceTrMatrix * model * vec4(vPosition, 1.0f);
}