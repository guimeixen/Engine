#version 450
#include "include/ubos.glsl"

layout(location = 0) in vec3 inPos;
#ifdef INSTANCING
layout(location = 3) in mat4 instanceMatrix;
#endif
#ifdef ANIMATED
layout(location = 3) in ivec4 boneIDs;
layout(location = 4) in vec4 weights;
#endif

#ifndef INSTANCING
uniform mat4 toWorldSpace;
uniform int instanceDataOffset;
#endif

void main()
{
	vec4 wPos;
	
#ifdef ANIMATED
	mat4 boneTransform = boneTransforms[boneIDs[0]] * weights[0];
	boneTransform += boneTransforms[boneIDs[1]] * weights[1];
	boneTransform += boneTransforms[boneIDs[2]] * weights[2];
	boneTransform += boneTransforms[boneIDs[3]] * weights[3];

	vec4 pos = boneTransform * vec4(inPos, 1.0);		// Transform from bone space to local space
#else
	vec4 pos = vec4(inPos, 1.0);
#endif

#ifdef INSTANCING
	wPos = instanceMatrix * pos;
	
	/*vec3 objPos = vec3(instanceMatrix[3][0], instanceMatrix[3][1], instanceMatrix[3][2]);
	
	wPos.xyz = MainBending(wPos.xyz, objPos);*/

#else
	if (instanceDataOffset == -1)
	{
		wPos = toWorldSpace * pos;	
	}
	else
	{
		mat4 t = transforms[instanceDataOffset + gl_InstanceID];
		wPos = t * pos;
	}
#endif

	gl_Position = projView * wPos;
}
