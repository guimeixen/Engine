#include "include/common.hlsli"

Texture2D tex : register(index0);
SamplerState samp : register(s1);

struct PixelInput
{
	float4 position : SV_POSITION;
	float2 uv : TEXCOORD0;
};

float4 PS(PixelInput i) : SV_TARGET
{
	float2 dim;
	tex.GetDimensions(dim.x, dim.y);
	float2 texelSize = 1.0 / dim; 	// gets size of single texel
    
	float4 offset = texelSize.xyxy * float4(-1.0, -1.0, 1.0, 1.0);
	
	float3 result = tex.Sample(samp, i.uv + offset.xy).rgb;
	result += tex.Sample(samp, i.uv + offset.zy).rgb;
	result += tex.Sample(samp, i.uv + offset.xw).rgb;
	result += tex.Sample(samp, i.uv + offset.zw).rgb;
	
	//result *= 1.0 / 4.0;
	result *= 0.25;
	
    return float4(result, 1.0);
}