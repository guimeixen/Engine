#include "include/ubos.hlsli"

cbuffer MaterialData : register(b3)
{
	uint startIndex;
	float4 color;
	float depth;
};

struct VertexInput
{
	float4 posUv : POSITION;
};

struct PixelInput
{
	float4 position : SV_POSITION;
	float2 uv : TEXCOORD0;
	float4 color : TEXCOORD1;
};

PixelInput VS(VertexInput i)
{
	PixelInput o;
	
	float4x4 toWorldSpace = GetModelMatrix(startIndex);

	o.position = mul(float4(i.posUv.x, i.posUv.y , 0.0, 1.0), toWorldSpace);
	o.position = mul(o.position, projView);
	o.uv = float2(i.posUv.z, i.posUv.w);
	o.color = color;

	return o;
}