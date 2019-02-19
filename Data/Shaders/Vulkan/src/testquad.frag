#version 450

layout(location = 0) out vec4 color;

layout (location = 0) in vec2 uv;

//layout(binding = 0, rgba8) uniform readonly image2D inputImg;
layout(set = 1, binding = 0) uniform sampler2D inputImg;

void main()
{
	//color = imageLoad(inputImg, ivec2(uv));
	color.rgb = texture(inputImg, uv).rrr;
	color.a=1.0;
	//color = vec4(1.0, 0.0, 0.3, 1.0);
}