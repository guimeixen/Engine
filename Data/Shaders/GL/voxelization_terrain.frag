#version 450

layout(location = 0) in vec2 uv;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec4 lightSpacePos;
layout(location = 3) flat in int axis;
layout(location = 4) flat in vec3 color;
layout(location = 5) in float roughness;
layout(location = 6) in vec3 worldPos;

#include include/ubos.glsl
#include include/voxelization_helpers.glsl

layout(binding = VOXEL_TEXTURE_SLOT, r32ui) uniform coherent volatile uimage3D voxelTexture;
layout(binding = SHADOW_MAP_SLOT) uniform sampler2DShadow shadowMap;

layout(binding = 2) uniform sampler2D roughnessMap;

void main()
{
	ivec3 dim = imageSize(voxelTexture);
	vec3 tempPos = vec3(gl_FragCoord.xy, gl_FragCoord.z * dim.z);
	vec3 voxelPos;
	
	if (axis == 1)
	{
		voxelPos.x = dim.x - tempPos.z;
		voxelPos.z = dim.x - tempPos.x;			// Flip the z otherwise the scene is flipped on the z axis
		voxelPos.y = tempPos.y;
	}
	else if (axis == 2)
	{
		voxelPos.z = dim.y - tempPos.y;
		voxelPos.y = dim.y - tempPos.z;		// Flip the z
		voxelPos.x = tempPos.x;
	}
	else
	{
		voxelPos.z = dim.z - tempPos.z;
		voxelPos.x = tempPos.x;
		voxelPos.y = tempPos.y;
	}
	
	vec3 N = normalize(normal);
	
	vec3 shadowUV = lightSpacePos.xyz;
	shadowUV.x = shadowUV.x * ONE_OVER_CASCADE_COUNT;
	float bias = 0.00065;
	shadowUV.z -= bias;
	float shadow = texture(shadowMap, shadowUV).r;
	
	vec3 diffuse = max(dot(N, dirAndIntensity.xyz), 0.0) * dirLightColor * dirAndIntensity.w;
	
	vec3 lighting = diffuse * shadow;
	
	// Point lights
	/*for (int i = 0; i < currentPointLights; i++)
	{
		vec3 wPosToLight = pointLights[i].posAndIntensity.xyz - worldPos;
		vec3 pLightDir = normalize(wPosToLight);
		vec3 pDiff = max(dot(N, pLightDir), 0.0) * pointLights[i].colorAndRadius.xyz * pointLights[i].posAndIntensity.w;

		float dist = length(wPosToLight);
		float att = clamp(1.0 - dist / pointLights[i].colorAndRadius.w, 0.0, 1.0);
		att *= att;
		pDiff *= att;

		lighting += pDiff;
	}*/
	
	//float roughness = texture(roughnessMap, uv).r;
	
	vec3 voxelColor = lighting * color * roughness;
	
	imageAtomicRGBA8Avg(voxelTexture, ivec3(voxelPos), vec4(voxelColor,1.0));
	
	//imageStore(voxelTexture, ivec3(voxelPos), vec4(color, 1.0));
}