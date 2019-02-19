#include "include/ubos.hlsli"

cbuffer MaterialData : register(b3)
{
	uint startIndex;
	float3 color;
};

struct VertexInput
{
	float3 pos : POSITION;
};

struct PixelInput
{
	float4 position : SV_POSITION;
	float3 color : TEXCOORD0;
};

PixelInput VS(VertexInput i)
{
	PixelInput o;
	
	float4x4 toWorldSpace = GetModelMatrix(startIndex);
	
	o.color = color;
	o.position = mul(float4(i.pos, 1.0), toWorldSpace);
	o.position = mul(o.position, projView);

	return o;
}