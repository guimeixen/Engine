#version 450

layout(location = 0) in vec4 posuv;

layout(location = 0) out vec2 uv;

uniform mat4 toWorldSpace;

#include include/ubos.glsl

layout(std140, binding = 3) uniform ObjectUBO
{
	vec4 color;
	float depth;
};

void main()
{
	uv = vec2(posuv.z, 1.0 - posuv.w);
	gl_Position = projView * toWorldSpace * vec4(posuv.xy, depth, 1.0);		// FIX DEPTH
}