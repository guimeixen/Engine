#version 450
#include include/ubos.glsl

layout(location = 0) out vec4 outColor;

layout(location = 0) in vec2 uv;
layout(location = 1) in float morph;
layout(location = 2) in vec3 worldPos;
layout(location = 3) in vec3 normal;
layout(location = 4) in float clipSpaceDepth;
layout(location = 5) in vec4 lightSpacePos[3];

// Heightmap uses the FIRST_SLOT
layout(binding = FIRST_SLOT + 1) uniform sampler2D diffuseR;
layout(binding = FIRST_SLOT + 2) uniform sampler2D diffuseG;
layout(binding = FIRST_SLOT + 3) uniform sampler2D diffuseB;
layout(binding = FIRST_SLOT + 4) uniform sampler2D diffuseBlack;
layout(binding = FIRST_SLOT + 5) uniform sampler2D normalR;
layout(binding = FIRST_SLOT + 6) uniform sampler2D splatmapTex;

#include include/shadow.glsl

#ifdef FORWARD_PLUS
#include include/forward_plus.glsl

layout(std430, binding = O_LIGHT_INDEX_LIST) readonly buffer OpaqueLightIndexList
{
	uint oLightIndexList[];
};

layout(binding = LIGHT_GRID_BINDING, rg32ui) uniform readonly uimage2D oLightGrid;
#endif

void main()
{
	vec3 lighting = vec3(0.0);
	outColor = vec4(0.0);
	
	vec2 newUV = uv * 300.0;
	
	vec3 N = normalize(normal);
	
	// Normal mapping
	vec3 fineNormal = texture(normalR, uv * 160.0).rbg;
	fineNormal = fineNormal * 2.0 - 1.0;		// From RGB [0,1] to [-1,1]
	//vec3 N = normal + fineNormal.x * tangent + fineNormal.y * bitangent;
	N += fineNormal;
	N = normalize(N);
	
	vec4 splatmap = texture(splatmapTex, uv);	
	float blackAmount = 1.0 - splatmap.r - splatmap.g - splatmap.b;
	
	vec3 redTexture = texture(diffuseR, newUV).rgb;
	vec3 greentexture = texture(diffuseG, newUV).rgb;
	vec3 blueTexture = texture(diffuseB, newUV).rgb;
	vec3 blackTexture = texture(diffuseBlack, newUV).rgb;
	vec3 blackTexture2 = texture(diffuseBlack, newUV * 0.25).rgb;

	redTexture *= splatmap.r;
	greentexture *= splatmap.g;
	blueTexture *= splatmap.b;
	blackTexture *= blackAmount;
	
	outColor.rgb = redTexture + greentexture + blueTexture + blackTexture;

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
	outColor.a = 1.0;
}