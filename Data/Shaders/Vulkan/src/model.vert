#version 450
#extension GL_GOOGLE_include_directive : enable
#include "include/ubos.glsl"

layout(location = 0) in vec3 inPos;
layout(location = 1) in vec2 inUv;
layout(location = 2) in vec3 inNormal;
#ifdef INSTANCING
layout(location = 3) in mat4 instanceMatrix;
#endif
#ifdef ANIMATED
layout(location = 3) in ivec4 boneIDs;
layout(location = 4) in vec4 weights;
#endif

#include "include/utils.glsl"

layout(location = 0) out vec2 uv;
layout(location = 1) out vec3 normal;
layout(location = 2) out vec3 worldPos;
layout(location = 3) out float clipSpaceDepth;
layout(location = 4) out vec4 lightSpacePos[3];

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
	vec4 wPos;
	
#ifdef ANIMATED
	mat4 boneTransform = GetBoneTransform(startIndex);
	vec4 pos = boneTransform * vec4(inPos, 1.0);		// Transform from bone space to local space
	vec4 N = boneTransform * vec4(inNormal, 0.0);
#else
	vec4 pos = vec4(inPos, 1.0);
	vec4 N = vec4(inNormal, 0.0);
#endif

#ifdef INSTANCING
	normal = (instanceMatrix * N).xyz;
	wPos = instanceMatrix * pos;
	vec3 objPos = vec3(instanceMatrix[3][0], instanceMatrix[3][1], instanceMatrix[3][2]);
	wPos.xyz = MainBending(wPos.xyz, objPos);
#else	
	mat4 toWorldSpace = GetModelMatrix(startIndex);
	normal = (toWorldSpace * N).xyz;
	wPos = toWorldSpace * pos;
#endif

	worldPos = wPos.xyz;

	gl_Position = projView * wPos;
	
	// Vertex position in light's clip space	range [-1,1] instead of [-w,w]. No need to divide by w because it's an orthographic projection and so w is unused
	lightSpacePos[0] = lightSpaceMatrix[0] * wPos;		
	lightSpacePos[1] = lightSpaceMatrix[1] * wPos;
	lightSpacePos[2] = lightSpaceMatrix[2] * wPos;
	
	clipSpaceDepth = gl_Position.w / nearFarPlane.y;
}