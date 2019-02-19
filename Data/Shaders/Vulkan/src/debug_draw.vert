#version 450
#extension GL_GOOGLE_include_directive : enable

layout(location = 0) in vec3 inPos;
layout(location = 1) in mat4 modelMatrix;
layout(location = 5) in vec3 inColor;

layout(location = 0) out vec3 color;

#include "include/ubos.glsl"

void main()
{
	color = inColor;
	gl_Position = projView * modelMatrix * vec4(inPos, 1.0);
}
