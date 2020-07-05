#version 450
#include "include/ubos.glsl"

layout(location = 0) in vec4 posuv;

layout(location = 0) out vec2 uv;

uniform mat4 toWorldSpace;

PROPERTIES
{
	vec4 color;
	float depth;
};

void main()
{
	uv = vec2(posuv.z, 1.0 - posuv.w);
	gl_Position = projView * toWorldSpace * vec4(posuv.xy, depth, 1.0);		// FIX DEPTH
}