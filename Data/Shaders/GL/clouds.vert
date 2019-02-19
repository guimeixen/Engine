#version 450

layout(location = 0) in vec4 posuv;

layout(location = 0) out vec2 uv;
layout(location = 1) out vec3 camRay;

#include include/ubos.glsl

vec3 UVToCamRay(vec2 uv)
{
	vec4 clipSpacePos = vec4(uv * 2.0 - 1.0, 1.0, 1.0);
	vec4 viewSpacePos = cloudsInvProjJitter * clipSpacePos;
	viewSpacePos.xyz /= viewSpacePos.w;
	vec4 worldSpacePos = invView * viewSpacePos;
	
	return worldSpacePos.xyz;
}

void main()
{
	uv = posuv.zw;
	camRay = UVToCamRay(uv);
	gl_Position = vec4(posuv.xy, 0.1, 1.0);
}