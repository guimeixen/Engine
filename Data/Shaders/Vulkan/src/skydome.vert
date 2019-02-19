#version 450
#extension GL_GOOGLE_include_directive : enable

#include "include/ubos.glsl"

layout(location = 0) in vec3 inPos;

layout(location = 0) out vec3 worldPos;

void main()
{
	mat4 m = mat4(1000.0);
	m[3] = vec4(camPos.xyz, 1.0);
	worldPos = (m * vec4(inPos, 1.0)).xyz;

	gl_Position = projView * m * vec4(inPos, 1.0);
	gl_Position = gl_Position.xyww;
}