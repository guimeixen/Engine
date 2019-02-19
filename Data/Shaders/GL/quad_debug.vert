#version 450

layout(location = 0) in vec4 posuv;

layout (location = 0) out vec2 uv;

layout(std140, binding = 1) uniform MaterialUBO
{
	float scale;
	vec2 trans;
};

#include include/ubos.glsl

void main()
{
	uv = vec2(posuv.z, posuv.w);
	//gl_Position = vec4(posuv.x * scale + trans.x, posuv.y * scale + trans.y, 0.0, 1.0);
	//gl_Position = vec4(posuv.xy, 0.0, 1.0);
	gl_Position = vec4(posuv.x * 0.45 + 0.45, posuv.y * 0.45 - 0.45, 0.0, 1.0);
}