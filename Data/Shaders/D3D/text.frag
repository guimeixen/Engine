#include "include/common.hlsli"

Texture2D rtTexture : register(index0);
SamplerState samp : register(s1);

struct PixelInput
{
	float4 position : SV_POSITION;
	float2 uv : TEXCOORD0;
	float4 color : TEXCOORD1;
};

// Large text
//const float width = 0.51;
//const float edgeWidth = 0.02;

// Medium text
//const float width = 0.485;
//const float edgeWidth = 0.10;

// Small text
static const float width = 0.46;
static const float edgeWidth = 0.19;

float4 PS(PixelInput i) : SV_TARGET
{
	float dist = 1.0 -  rtTexture.Sample(samp, i.uv).a;	// The further a pixel is the higher the distance from the center. The alpha is the opposite it decreases the further from the center
															// For the correct distance we need to invert the alpha
															
	float alpha = 1.0 - smoothstep(width, width + edgeWidth, dist);	
	float4 col = float4(i.color.rgb, alpha * i.color.a);
	
	return col;
}
