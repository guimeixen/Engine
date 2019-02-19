#include "include/common.hlsli"

Texture2D rtTexture : register(index0);
SamplerState samp : register(s1);

struct PixelInput
{
	float4 position : SV_POSITION;
	float2 uv : TEXCOORD0;
};

float4 PS(PixelInput i) : SV_TARGET
{
	float3 col = rtTexture.Sample(samp, i.uv).rrr;

	return float4(col,1.0);
}