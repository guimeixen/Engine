#version 450

layout(location = 0) in vec3 pos;

uniform mat4 toWorldSpace;

#include include/ubos.glsl

void main()
{
	gl_Position = projView * toWorldSpace * vec4(pos, 1.0);
}