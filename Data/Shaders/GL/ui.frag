#version 450
#include "../common.glsl"

layout(location = 0) out vec4 outColor;

layout(location = 0) in vec2 uv;

tex2D_u(0) tex;

PROPERTIES
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