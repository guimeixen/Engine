#version 450
#extension GL_GOOGLE_include_directive : enable
#include "include/ubos.glsl"

layout(location = 0) in vec3 inPos;
layout(location = 1) in vec2 inUv;
layout(location = 2) in vec3 inNormal;

layout(location = 0) out vec3 normal;

PROPERTIES
{
	float scale;
};

void main()
{
	normal = inNormal;

	mat4 scaleMatrix = mat4(1.0);
	scaleMatrix[0][0] = scale;
	scaleMatrix[1][1] = scale;
	scaleMatrix[2][2] = scale;
	gl_Position = projView * scaleMatrix * vec4(inPos, 1.0);
}