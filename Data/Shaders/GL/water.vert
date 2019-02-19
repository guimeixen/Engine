#version 450

layout(location = 0) in vec3 inPos;
layout(location = 1) in vec2 inUv;

out vec2 uv;
out vec4 clipSpacePos;
out vec3 worldPos;

#include include/ubos.glsl

uniform mat4 toWorldSpace;

void main()
{
	uv = inUv * 16.0;
	
	vec4 wPos = toWorldSpace * vec4(inPos, 1.0);
	worldPos = wPos.xyz;
	
	clipSpacePos = projView * wPos;
	gl_Position = clipSpacePos;
}