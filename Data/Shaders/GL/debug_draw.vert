#version 450
#include "include/ubos.glsl"

layout(location = 0) in vec3 inPos;
layout(location = 1) in mat4 modelMatrix;
layout(location = 5) in vec3 inColor;

out vec3 color;

void main()
{
	color = inColor;
	gl_Position = projView * modelMatrix * vec4(inPos, 1.0);
}