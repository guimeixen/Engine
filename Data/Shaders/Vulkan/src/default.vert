#version 450
#extension GL_GOOGLE_include_directive : enable

layout(location = 0) in vec3 inPos;

#include "include/ubos.glsl"

void main()
{
	gl_Position = projView * vec4(inPos, 1.0);
}