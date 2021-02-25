#version 450

layout(location = 0) in vec3 inPos;
layout(location = 1) in vec2 inUv;
layout(location = 2) in vec3 inNormal;
#ifdef INSTANCING
layout(location = 3) in mat4 instanceMatrix;
#endif

layout(location = 0) out vec2 uvGeom;
layout(location = 1) out vec3 normalGeom;
layout(location = 2) out vec3 worldPosGeom;
layout(location = 3) out vec4 lightSpacePosGeom;

#ifndef INSTANCING
uniform mat4 toWorldSpace;
uniform int instanceDataOffset;
#endif

#include "include/ubos.glsl"

void main()
{
	uvGeom = inUv;
	vec4 wPos;
	
	vec4 pos = vec4(inPos, 1.0);
	vec4 N = vec4(inNormal, 0.0);
	
#ifdef INSTANCING
	//normal = mat3(transpose(inverse(instanceMatrix))) * inNormal;		// Don't allow non-uniform scaling so we don't do this every frame
	normalGeom = (instanceMatrix * N).xyz;
	wPos = instanceMatrix * pos;
#else
	if (instanceDataOffset == -1)
	{
		normalGeom = (toWorldSpace * N).xyz;
		wPos = toWorldSpace * pos;
	}
	else
	{
		mat4 t = transforms[instanceDataOffset + gl_InstanceID];
		normalGeom = (t* N).xyz;
		wPos = t * pos;
	}
#endif
	
	worldPosGeom = wPos.xyz;
	
	gl_Position = wPos;
	
	lightSpacePosGeom = lightSpaceMatrix[0] * wPos;
	//lightSpacePosGeom.xyz = lightSpacePosGeom.xyz / lightSpacePosGeom.w;
	lightSpacePosGeom.xy = lightSpacePosGeom.xy * 0.5 + 0.5;		// Only multiply the xy because the z is already in [0,1] (set with glClipControl)
	//lightSpacePos[0] = lightSpaceMatrix[0] * wPos;		
	//lightSpacePos[1] = lightSpaceMatrix[1] * wPos;
	//lightSpacePos[2] = lightSpaceMatrix[2] * wPos;

	//clipSpaceDepth = gl_Position.w / nearFarPlane.y;
}

