#include "include/common.hlsli"

Texture2D brightTexture : register(index0);
SamplerState brightSampler : register(s1);

Texture2D baseTexture : register(index1);
SamplerState baseSampler : register(s2);

struct PixelInput
{
	float4 position : SV_POSITION;
	float2 uv : TEXCOORD0;
};

float4 PS(PixelInput i) : SV_TARGET
{
	float2 dim;
	brightTexture.GetDimensions(dim.x, dim.y);
	float2 texelSize = 1.0 / dim; 	// gets size of single texel
    
	float4 offset = texelSize.xyxy * float4(-1.0, -1.0, 1.0, 1.0) * (1.0 * 0.5);
	float3 result = brightTexture.Sample(brightSampler, i.uv + offset.xy).rgb;
	result += brightTexture.Sample(brightSampler, i.uv + offset.zy).rgb;
	result += brightTexture.Sample(brightSampler, i.uv + offset.xw).rgb;
	result += brightTexture.Sample(brightSampler, i.uv + offset.zw).rgb;
	
	result *= 0.25;
	
	float3 base =  baseTexture.Sample(baseSampler, float2(i.uv.x, 1.0-i.uv.y)).rgb;
	
    return float4(base + result, 1.0);
}