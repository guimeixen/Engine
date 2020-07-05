#version 450
#include "include/ubos.glsl"

layout(location = 0) in vec3 pos;

uniform mat4 toWorldSpace;

void main()
{
	gl_Position = projView * toWorldSpace * vec4(pos, 1.0);
}