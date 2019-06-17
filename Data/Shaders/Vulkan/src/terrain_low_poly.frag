#version 450
#extension GL_GOOGLE_include_directive : enable
#include "include/ubos.glsl"

layout(location = 0) out vec4 outColor;

layout(location = 0) in vec2 uv;
layout(location = 1) in vec3 worldPos;
layout(location = 2) flat in vec3 normal;
layout(location = 3) in vec3 smoothNormal;
layout(location = 4) in float clipSpaceDepth;
layout(location = 5) in vec3 color;
layout(location = 6) in vec4 lightSpacePos[3];

#include "include/shadow.glsl"

#ifdef FORWARD_PLUS
#include "include/forward_plus.glsl"

layout(std140, set = 0, binding = O_LIGHT_INDEX_LIST) readonly buffer OpaqueLightIndexList
{
	uint oLightIndexList[];
};

layout(set = 0, binding = LIGHT_GRID_BINDING, rg32ui) uniform readonly uimage2D oLightGrid;
#endif

void main()
{
	vec3 lighting = vec3(0.0);
	outColor.rgb = color;

	vec3 N = normalize(smoothNormal);
	//vec3 smoothN = normalize(smoothNormal);

	// Directional light
	float NdotL = dot(N, dirAndIntensity.xyz);

	float shadow = calcShadow(clipSpaceDepth, NdotL);

	vec3 diff = vec3(max(NdotL, 0.0)) * dirLightColor.rgb * dirAndIntensity.w;
	lighting += diff * shadow + vec3(dirLightColor.a);

	// Point lights
#ifdef FORWARD_PLUS
	// Get the index of the current pixel in the light grid
	ivec2 tileIndex = ivec2(floor(gl_FragCoord.xy / TILE_SIZE));
	
	// Get the start offset and the light count for this pixel in the light index list
	uvec2 offsetAndLightCount = imageLoad(oLightGrid, tileIndex).xy;
	
	for (uint i = 0; i < offsetAndLightCount.y; i++)
	{
		uint lightIndex = oLightIndexList[offsetAndLightCount.x + i];
		Light light = lights[lightIndex];
		
		vec3 wPosToLight = light.posAndIntensityWS.xyz - worldPos;
		vec3 L = normalize(wPosToLight);
		vec3 diff = vec3(max(dot(N,L), 0.0)) * light.colorAndRadius.xyz * light.posAndIntensityWS.w;
		
		float dist = length(wPosToLight);
		float att = clamp(1.0 - dist / light.colorAndRadius.w, 0.0, 1.0);
		att *= att;
		diff *= att;
		
		lighting += diff;
	}
#else
	for (int i = 0; i < currentPointLights; i++)
	{
		vec3 wPosToLight = pointLights[i].posAndIntensity.xyz - worldPos;
		vec3 pLightDir = normalize(wPosToLight);
		vec3 pDiff = max(dot(N, pLightDir), 0.0) * pointLights[i].colorAndRadius.xyz * pointLights[i].posAndIntensity.w;

		float dist = length(wPosToLight);
		float att = clamp(1.0 - dist / pointLights[i].colorAndRadius.w, 0.0, 1.0);
		att *= att;
		pDiff *= att;

		lighting += pDiff;
	}
#endif	

	outColor.rgb *= lighting;
	//outColor.rgb = mix(outColor.rgb, projectedTexel.rgb, projectedTexel.a);
	outColor.a = 1.0;
	
#ifdef EDITOR
	float dist = length(worldPos.xz - terrainEditParams.xy);
	if(dist < 10.0 && dist > 9.0)
		outColor.rgb = vec3(1.0,0.0,0.0);
#endif
	
	// Heightmap display
	//float c = texture(heightmap, uv).r / 32.0;
	//outColor = vec4(outColor.r + c, c, c, 1.0);
}