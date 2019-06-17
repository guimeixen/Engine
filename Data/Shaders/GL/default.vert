#version 450

layout(location = 0) in vec3 inPos;

#include include/ubos.glsl

uniform mat4 toWorldSpace;

void main()
{
	gl_Position = projView * toWorldSpace *  vec4(inPos, 1.0);
}