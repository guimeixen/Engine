#version 450
#include "include/ubos.glsl"

layout(location = 0) in vec2 uv;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec3 worldPos;
layout(location = 3) in vec4 lightSpacePos;
layout(location = 4) flat in int axis;

//#include include/voxelization_helpers.glsl

tex2D_u(0) texDiffuse;
image3D_g(VOXEL_IMAGE, rgba8, writeonly) voxelTexture;
tex2DShadow_g(CSM_TEXTURE) shadowMap;

/*layout(binding = FIRST_SLOT) uniform sampler2D texDiffuse;
layout(binding = 0, rgba8) uniform writeonly image3D voxelTexture;
layout(binding = SHADOW_MAP_SLOT) uniform sampler2DShadow shadowMap;*/

void main()
{
	ivec3 dim = imageSize(voxelTexture);
	vec3 tempPos = vec3(gl_FragCoord.xy, gl_FragCoord.z * 128.0);		// For some reason dim.z is not working
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
	
	vec3 diffuse = max(dot(N, dirAndIntensity.xyz), 0.0) * dirLightColor.rgb * dirAndIntensity.w;
	
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
	
/*#ifdef ALPHA
	vec3 color = vec3(0.01, 0.793, 0.696) * 2.5;
#else
	vec3 color = lighting * texture(texDiffuse, uv).rgb/* + vec3(1.0,0.0,0.0);
#endif*/

	vec3 color = lighting * texture(texDiffuse, uv).rgb;
	imageStore(voxelTexture, ivec3(voxelPos), vec4(color, 1.0));
	//imageAtomicRGBA8Avg(voxelTexture, ivec3(voxelPos), vec4(color,1.0));
}