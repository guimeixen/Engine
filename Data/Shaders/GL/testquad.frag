#version 450
#include include/common.glsl

layout(location = 0) out vec4 color;

layout (location = 0) in vec2 uv;

//layout(binding = 0, rgba8) uniform readonly image2D inputImg;
layout(binding = 0) uniform sampler2D inputImg;

void main()
{
	//color = imageLoad(inputImg, ivec2(uv));
	color = texture(inputImg, uv);
}