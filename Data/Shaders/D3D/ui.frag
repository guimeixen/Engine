#include "include/common.hlsli"

Texture2D tex : register(index0);
SamplerState samp : register(s1);

struct PixelInput
{
	float4 position : SV_POSITION;
	float2 uv : TEXCOORD0;
	float4 color : TEXCOORD1;
};

float4 PS(PixelInput i) : SV_TARGET
{
	float4 col = tex.Sample(samp, i.uv) * i.color;
	
	return col;
}
