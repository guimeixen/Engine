#version 450

layout(location = 0) in vec3 inPos;
layout(location = 1) in mat4 modelMatrix;
layout(location = 5) in vec3 inColor;

out vec3 color;

#include include/ubos.glsl

void main()
{
	color = inColor;
	gl_Position = projView * modelMatrix * vec4(inPos, 1.0);
}