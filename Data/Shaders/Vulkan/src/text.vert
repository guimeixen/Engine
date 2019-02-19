#version 450
#extension GL_GOOGLE_include_directive : enable
#include "include/ubos.glsl"


layout(location = 0) in vec4 posuv;
layout(location = 1) in vec4 inColor;

layout(location = 0) out vec2 uv;
layout(location = 1) out vec4 color;

void main()
{
	color = inColor;
	uv = vec2(posuv.z, posuv.w);
	gl_Position =  projView * vec4(posuv.xy, 0.0, 1.0);
	//gl_Position = vec4(posuv.xy, 0.0, 1.0);
}