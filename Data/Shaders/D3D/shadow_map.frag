#include "include/common.hlsli"

Texture2D tex : register(index0);
SamplerState samp : register(s1);

struct PixelInput
{
	float4 position : SV_POSITION;
	float2 uv : TEXCOORD0;
};

void PS(PixelInput i) 
{
	float alpha = tex.Sample(samp, i.uv).a;	// The further a pixel is the higher the distance from the center. The alpha is the opposite it decreases the further from the center
															// For the correct distance we need to invert the alpha
	if (alpha < 0.5)						
		discard;
}
