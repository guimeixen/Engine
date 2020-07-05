#version 450
#extension GL_GOOGLE_include_directive : enable
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

#include "include/utils.glsl"

layout(location = 0) out vec2 uv;

PROPERTIES
{
	uint startIndex;
	uint numVecs;
};

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
	
#ifdef ANIMATED
	vec4 pos = GetBoneTransform(startIndex) * vec4(inPos, 1.0);		// Transform from bone space to local space
#else
	vec4 pos = vec4(inPos, 1.0);
#endif

	vec4 wPos;

#ifdef INSTANCING
	wPos = instanceMatrix * pos;
	vec3 objPos = vec3(instanceMatrix[3][0], instanceMatrix[3][1], instanceMatrix[3][2]);
	wPos.xyz = MainBending(wPos.xyz, objPos);
#else	
	wPos = GetModelMatrix(startIndex) * pos;
#endif
		
	gl_Position = projView * wPos;
}