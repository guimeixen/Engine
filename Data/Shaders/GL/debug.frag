#version 450

#include include/common.glsl

layout(location = 0) out vec4 outColor;

layout(std140, binding = MAT_UBO_BINDING) uniform MaterialUBO
{
	vec3 color;
};

void main()
{
	outColor.rgb = color;
	outColor.a = 1.0;
}