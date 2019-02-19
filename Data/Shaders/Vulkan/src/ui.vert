#version 450
#extension GL_GOOGLE_include_directive : enable
#include "include/ubos.glsl"
#include "include/utils.glsl"

layout(location = 0) in vec4 posuv;

layout (location = 0) out vec2 uv;

layout(push_constant) uniform PushConsts
{
	uint startIndex;
	uint numVecs;
	vec4 color;
	float depth;
};

void main()
{
	uv = vec2(posuv.z, 1.0 - posuv.w);
	gl_Position = projView * GetModelMatrix(startIndex) * vec4(posuv.xy, depth, 1.0);
}