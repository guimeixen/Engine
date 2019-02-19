#version 330 core
#extension GL_ARB_shading_language_420pack: enable

layout(location = 0) in vec3 inPos;
layout(location = 1) in vec2 inUv;
#ifdef INSTANCING
layout(location = 3) in mat4 instanceMatrix;
#endif
#ifdef ANIMATED
layout(location = 3) in ivec4 boneIDs;
layout(location = 4) in vec4 weights;
#endif

out vec2 uv;

#ifndef INSTANCING
uniform mat4 toWorldSpace;
#endif

#include include/ubos.glsl

#ifdef ANIMATED
layout(std140, binding = OBJECT_UBO_BINDING) uniform ObjectUBO
{
	mat4 boneTransforms[32];
};
#endif

void main()
{
	uv = inUv;

#ifdef ANIMATED
	mat4 boneTransform = boneTransforms[boneIDs[0]] * weights[0];
	boneTransform += boneTransforms[boneIDs[1]] * weights[1];
	boneTransform += boneTransforms[boneIDs[2]] * weights[2];
	boneTransform += boneTransforms[boneIDs[3]] * weights[3];

	vec4 pos = boneTransform * vec4(inPos, 1.0);		// Transform from bone space to local space
#else
	vec4 pos = vec4(inPos, 1.0);
#endif
	
	vec4 wPos;
	
#ifdef INSTANCING
	wPos = instanceMatrix * pos;
#else
	wPos = toWorldSpace * pos;
#endif

	gl_Position = projView * wPos;
}