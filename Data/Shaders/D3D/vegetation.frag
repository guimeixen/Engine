#include "include/common.hlsli"

Texture2D tex : register(index0);
SamplerState samp : register(s1);

struct PixelInput
{
	float4 position : SV_POSITION;
	float2 uv : TEXCOORD0;
	float3 worldPos : TEXCOORD1;
	float3 normal : TEXCOORD2;
};

float4 PS(PixelInput i) : SV_TARGET
{
	float3 N = normalize(i.normal);

	float4 diffuseMap = tex.Sample(samp, i.uv);
	
	if (diffuseMap.a < 0.35)
		discard;

	diffuseMap.a = 1.0;
	
	return diffuseMap;
}
