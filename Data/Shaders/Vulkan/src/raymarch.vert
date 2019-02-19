#version 450
#extension GL_GOOGLE_include_directive : enable

layout(location = 0) in vec3 inPos;
layout(location = 1) in vec2 inUv;

layout(location = 0) out vec3 worldPos;
layout(location = 1) out vec2 uv;

#include "include/ubos.glsl"

layout(push_constant) uniform PushConts
{
	mat4 modelMatrix;
};

void main()
{
	uv = inUv;
	vec4 wPos = modelMatrix * vec4(inPos , 1.0);
	worldPos = wPos.xyz;
	gl_Position = projView * wPos;
}