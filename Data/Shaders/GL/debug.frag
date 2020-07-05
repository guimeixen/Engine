#version 450
#include "../common.glsl"

layout(location = 0) out vec4 outColor;

PROPERTIES
{
	vec3 color;
};

void main()
{
	outColor.rgb = color;
	outColor.a = 1.0;
}