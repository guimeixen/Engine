#version 450
#extension GL_GOOGLE_include_directive : enable
#include "include/ubos.glsl"

layout(location = 0) in vec3 inPos;
#ifdef INSTANCING
layout(location = 3) in mat4 instanceMatrix;
#endif
#ifdef ANIMATED
layout(location = 3) in ivec4 boneIDs;
layout(location = 4) in vec4 weights;
#endif

#include "include/utils.glsl"

layout(push_constant) uniform PushConsts
{
	uint startIndex;
	uint numVecs;
};

void main()
{
	vec4 wPos;
	
#ifdef ANIMATED
	mat4 boneTransform = GetBoneTransform(startIndex);
	vec4 pos = boneTransform * vec4(inPos, 1.0);		// Transform from bone space to local space
#else
	vec4 pos = vec4(inPos, 1.0);
#endif

#ifdef INSTANCING
	wPos = instanceMatrix * pos;
#else	
	mat4 toWorldSpace = GetModelMatrix(startIndex);
	wPos = toWorldSpace * pos;
#endif

	gl_Position = projectionMatrix *  viewMatrix * wPos;
}
