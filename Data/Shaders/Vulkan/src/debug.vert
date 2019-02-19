#version 450
#extension GL_GOOGLE_include_directive : enable

#include "include/ubos.glsl"
#include "include/utils.glsl"

layout(location = 0) in vec3 pos;

layout(location = 0) out vec3 col;

layout(push_constant) uniform PushConsts
{
	uint startIndex;
	uint numVecs;
	vec3 color;
};

void main()
{
	col = color;
	mat4 toWorldSpace = GetModelMatrix(startIndex);
	
	gl_Position =  projView * toWorldSpace * vec4(pos, 1.0);
}
