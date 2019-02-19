#include "include/common.hlsli"

Texture2D inputImg : register(t1);
SamplerState samp : register(s1);

struct PixelInput
{
	float4 position : SV_POSITION;
	float2 uv : TEXCOORD0;
};

float4 PS(PixelInput i) : SV_TARGET
{
	float3 col = inputImg.Sample(samp, i.uv).rgb;

	return float4(col,1.0);
}