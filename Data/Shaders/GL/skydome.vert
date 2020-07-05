#version 450
#include "include/ubos.glsl"

layout(location = 0) in vec3 inPos;

layout(location = 0) out vec3 worldPos;

void main()
{
	mat4 m = mat4(1000.0);
	m[3] = vec4(camPos.xyz, 1.0);
	worldPos = (m * vec4(inPos, 1.0)).xyz;

	//mat4 m = mat4(1.0);
	//m[3] = vec4(camPos, 1.0);
	gl_Position = projView * m * vec4(inPos, 1.0);
	gl_Position = gl_Position.xyww;
}