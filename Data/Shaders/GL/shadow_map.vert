#version 450
#include "include/ubos.glsl"

layout(location = 0) in vec3 inPos;
layout(location = 1) in vec2 inUv;
#ifdef INSTANCING
layout(location = 3) in mat4 instanceMatrix;		// Use #ifdef INSTANCING or use the if below?
#endif
#ifdef ANIMATED
layout(location = 3) in ivec4 boneIDs;
layout(location = 4) in vec4 weights;
#endif

layout(location = 0) out vec2 uv;

#ifndef INSTANCING
uniform mat4 toWorldSpace;
uniform int instanceDataOffset;
#endif

#ifdef ANIMATED
layout(std140, binding = OBJECT_UBO_BINDING) uniform ObjectUBO
{
	mat4 boneTransforms[32];
};
#endif

#ifdef INSTANCING

const float bendScale = 0.015;

vec3 MainBending(vec3 worldPos, vec3 objPos)
{
	vec3 vPos = worldPos - objPos;
	
	// Calculate the length fromt the ground
	float lengthFromGround = length(vPos);
	// Bend factor - wind variation done on the cpu
	float bf = vPos.y * bendScale;
	// Smooth bending factor and increase its nearby height limit
	bf += 1.0;
	bf *= bf;
	bf = bf * bf - bf;
	// Displace position
	
	//vec2 windDirStrr = vec2(sin(timeElapsed), cos(timeElapsed));
	vPos.xz += windDirStr.xy * bf;
	// Rescale - this keeps the plants parts form stretching by shortening the y (height) while
	// they move about the xz
	vPos = normalize(vPos) * lengthFromGround;
	vPos += objPos;
	
	return vPos;
}
#endif

void main()
{
	uv = inUv;
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
	vec3 objPos = vec3(instanceMatrix[3][0], instanceMatrix[3][1], instanceMatrix[3][2]);	
	wPos.xyz = MainBending(wPos.xyz, objPos);
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