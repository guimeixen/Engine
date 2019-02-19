#version 450

layout(location = 0) in vec3 pos;

out vec3 uv;

#include include/ubos.glsl

void main()
{
	uv = pos;
	mat4 m = mat4(1.0);
	m[3] = vec4(camPos.xyz, 1.0);
	gl_Position = projView * m * vec4(pos, 1.0);
	gl_Position = gl_Position.xyww;
}