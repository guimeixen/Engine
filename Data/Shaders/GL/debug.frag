#version 450

#include include/common.glsl

layout(location = 0) out vec4 outColor;

/*layout(std140, binding = MAT_UBO_BINDING) uniform MaterialUBO
{
	vec3 color;
};*/

void main()
{
	outColor.rgb = vec3(1.0);
	outColor.a = 1.0;
}