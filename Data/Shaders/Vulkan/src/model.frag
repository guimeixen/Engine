#version 450
#extension GL_GOOGLE_include_directive : enable
#include "include/ubos.glsl"

layout(location = 0) out vec4 outColor;

layout(location = 0) in vec2 uv;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec3 worldPos;
layout(location = 3) in float clipSpaceDepth;
layout(location = 4) in vec4 lightSpacePos[3];

#include "include/shadow.glsl"
#include "include/voxel_cone_tracing.glsl"

//layout(set = 2, binding = 0) uniform sampler2D tex;
tex2D_u(0) tex;

#ifdef FORWARD_PLUS
#include "include/forward_plus.glsl"

layout(std140, set = 0, binding = O_LIGHT_INDEX_LIST) readonly buffer OpaqueLightIndexList
{
	uint oLightIndexList[];
};

uimage2D_g(LIGHT_GRID_BINDING, rg32ui, readonly) oLightGrid;
//layout(set = 0, binding = LIGHT_GRID_BINDING, rg32ui) uniform readonly uimage2D oLightGrid;
#endif

void main()
{
	outColor = texture(tex, uv);
	
	if (outColor.a < 0.35)
		discard;
	
	vec3 N = normalize(normal);
	vec3 V = normalize(camPos.xyz - worldPos);
	vec3 H = normalize(dirAndIntensity.xyz + V);
	vec3 lighting = vec3(0.0);
	
	float NdotL = dot(N, dirAndIntensity.xyz);
	float shadow = calcShadow(clipSpaceDepth, NdotL);
	vec3 diff = max(NdotL, 0.0) * dirLightColor.xyz * dirAndIntensity.w;
	lighting += diff * shadow;
	lighting += vec3(dirLightColor.w);		// Ambient

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
	
	vec4 indirectDiffuse = indirectDiffuseLight(N);
	lighting += indirectDiffuse.rgb;
	lighting += skyColor * skyColorMultiplier;
	//lighting *= indirectDiffuse.a;
	
	outColor.rgb *= lighting;
	//outColor.rgb = indirectDiffuse.rgb;
	outColor.a = 1.0;
}