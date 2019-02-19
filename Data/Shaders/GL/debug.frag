#version 450

layout(location = 0) out vec4 outColor;

layout(std140, binding = 1) uniform MaterialUBO
{
	vec3 color;
};

void main()
{
	outColor.rgb = color;
	outColor.a = 1.0;
}