#version 450
#include include/common.glsl

layout(location = 0) out vec4 outColor;

layout(location = 0) in vec2 uv;

layout(binding = FIRST_SLOT) uniform sampler2D tex;

layout(std140, binding = 3) uniform ObjectUBO
{
	vec4 color;
	float depth;
};

void main()
{
	vec4 col = texture(tex, uv);
	//if(col.a < 0.5)
	//	discard;
	outColor = col * color;
}