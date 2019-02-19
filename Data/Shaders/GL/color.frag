#version 450

out vec4 color;

in vec2 uv;

layout(binding = 1) uniform sampler2D tex;
uniform vec4 col;

// Large text
//const float width = 0.51;
//const float edgeWidth = 0.02;

// Small text
const float width = 0.46;
const float edgeWidth = 0.19;

void main()
{
	color = texture(tex, uv) * col;
}