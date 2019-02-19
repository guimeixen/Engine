#include "include/ubos.hlsli"
#include "include/shadow.hlsli"

struct PixelInput
{
	float4 position : SV_POSITION;
	float2 uv : TEXCOORD0;
	float3 worldPos : TEXCOORD1;
	float3 normal : TEXCOORD2;
	float clipSpaceDepth : TEXCOORD3;
	float4 lightSpacePos[3] : TEXCOORD4;
};

Texture2D diffuseR : register(index1);
SamplerState diffuseRSampler : register(s2);

float4 PS(PixelInput i) : SV_TARGET
{
	float4 outColor;
	
	float3 lighting = (0.0).xxx;
	float2 newUV = i.uv * 200.0;
	
	float3 N = normalize(i.normal);
	
	float3 redTexture = diffuseR.Sample(diffuseRSampler, newUV).rgb;
	
	outColor.rgb = redTexture;
	
	float NdotL = dot(N, dirAndIntensity.xyz);

	float shadow = calcShadow(i.clipSpaceDepth, NdotL, i.lightSpacePos);
	
	float3 diff = max(NdotL, 0.0) * dirLightColor.rgb * dirAndIntensity.w;
	lighting += diff * shadow + (dirLightColor.a).xxx;

	// Point lights
	for (int j = 0; j < currentPointLights; j++)
	{
		float3 wPosToLight = pointLights[j].posAndIntensity.xyz - i.worldPos;
		float3 pLightDir = normalize(wPosToLight);
		float3 pDiff = max(dot(N, pLightDir), 0.0) * pointLights[j].colorAndRadius.xyz * pointLights[j].posAndIntensity.w;

		float dist = length(wPosToLight);
		float att = clamp(1.0 - dist / pointLights[j].colorAndRadius.w, 0.0, 1.0);
		att *= att;
		pDiff *= att;

		lighting += pDiff;
	}
	
	outColor.rgb *= lighting;
	outColor.a = 1.0;
	
	return outColor;
}