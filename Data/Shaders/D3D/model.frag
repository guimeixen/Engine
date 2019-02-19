#include "include/ubos.hlsli"
#include "include/shadow.hlsli"

Texture2D tex : register(index0);
SamplerState samp : register(s1);

struct PixelInput
{
	float4 position : SV_POSITION;
	float2 uv : TEXCOORD0;
	float3 worldPos : TEXCOORD1;
	float3 normal : TEXCOORD2;
	float clipSpaceDepth : TEXCOORD3;
	float3 lightSpacePos[3] : TEXCOORD4;
};

float4 PS(PixelInput pi) : SV_TARGET
{
	float3 N = normalize(pi.normal);
	float3 lighting = (0.0).xxx;
	float4 outColor = tex.Sample(samp, pi.uv);
	
	if (outColor.a < 0.35)
		discard;
		
	float NdotL = dot(N, dirAndIntensity.xyz);
	float shadow = calcShadow(pi.clipSpaceDepth, NdotL, pi.lightSpacePos);
	float3 diff = max(NdotL, 0.0) * dirLightColor.xyz * dirAndIntensity.w;
	lighting += diff * shadow;
	lighting += (dirLightColor.w).xxx;		// Ambient
	
	// Point lights
	for (int i = 0; i < currentPointLights; i++)
	{
		float3 wPosToLight = pointLights[i].posAndIntensity.xyz - pi.worldPos;
		float3 pLightDir = normalize(wPosToLight);
		float3 pDiff = max(dot(N, pLightDir), 0.0) * pointLights[i].colorAndRadius.xyz * pointLights[i].posAndIntensity.w;

		float dist = length(wPosToLight);
		float att = saturate(1.0 - dist / pointLights[i].colorAndRadius.w);
		att *= att;
		pDiff *= att;

		lighting += pDiff;
	}

	outColor.rgb *= lighting;
	outColor.a = 1.0;
	
	return outColor;
}