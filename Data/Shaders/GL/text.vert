#version 450

layout(location = 0) in vec4 posuv;
layout(location = 1) in vec4 inColor;

out vec2 uv;
out vec4 color;

#include include/ubos.glsl

void main()
{
	color = inColor;
	uv = vec2(posuv.z, posuv.w);
	gl_Position = projView * vec4(posuv.xy, 0.0, 1.0);
}