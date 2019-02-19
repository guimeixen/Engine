#version 450
#include include/common.glsl

out vec4 color;

in vec2 uv;

layout(binding = FIRST_SLOT) uniform sampler2D tex;

void main()
{
	color = texture(tex, uv);
}