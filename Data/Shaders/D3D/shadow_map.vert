#include "include/ubos.hlsli"

cbuffer MaterialData : register(b1)
{
	uint startIndex;
};

struct VertexInput
{
	float3 pos : POSITION;
	float2 uv : NORMAL;
};

struct PixelInput
{
	float4 position : SV_POSITION;
	float2 uv : TEXCOORD0;
};

PixelInput VS(VertexInput i)
{
	PixelInput o;
	o.uv = i.uv;
	
	float4x4 toWorldSpace = GetModelMatrix(startIndex);
	
	float4 wPos = mul(float4(i.pos, 1.0), toWorldSpace);
	o.position = mul(wPos, projView);
	
	return o;
}