#version 450
#extension GL_GOOGLE_include_directive : enable

layout(location = 0) in vec3 inPos;
layout(location = 1) in float intensity;

layout(location = 0) out float heightGradient;
layout(location = 1) out float branchIntensity;

#include "include/ubos.glsl"
#include "include/utils.glsl"

void main()
{
	branchIntensity = intensity;
	//mat4 modelMatrix = GetModelMatrix();
	//vec3 objPos = vec3(modelMatrix[3][0], modelMatrix[3][1], modelMatrix[3][2]);
	//objPos.y += 16.0;
	//vec4 wPos = modelMatrix * vec4(inPos, 1.0);
	vec3 objPos = vec3(0.0);
	vec4 wPos = vec4(inPos, 1.0);
	vec3 worldPos = wPos.xyz;
	heightGradient = length(worldPos - objPos) / 32.0;		// 32.0 = lightning strike height
	gl_Position = projView * wPos;
}