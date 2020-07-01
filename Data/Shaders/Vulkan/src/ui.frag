#version 450
#extension GL_GOOGLE_include_directive : enable
#include "include/common.glsl"

layout(location = 0) out vec4 outColor;

layout (location = 0) in vec2 uv;

tex_bind2D_user(0) tex;
//layout(set = 1, binding = 0) uniform sampler2D tex;

layout(push_constant) uniform PushConsts
{
	uint startIndex;
	uint numVecs;
	vec4 color;
	float depth;
};

void main()
{
	outColor = texture(tex, uv) * color;
}