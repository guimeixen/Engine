#include "include/ubos.hlsli"

struct VertexInput
{
	float3 pos : POSITION;
};

struct PixelInput
{
	float4 position : SV_POSITION;
	float3 worldPos : TEXCOORD1;
};

PixelInput VS(VertexInput i)
{
	PixelInput o;
	
	float4x4 m = {
	1000.0, 0.0, 0.0, 0.0,
	0.0, 1000.0, 0.0, 0.0,
	0.0, 0.0, 1000.0, 0.0,
	camPos.x, camPos.y, camPos.z, 1};
	
	float4 wPos = mul(float4(i.pos, 1.0), m);
	o.worldPos = wPos.xyz;
	
	o.position = mul(wPos, projView);
	o.position.z = o.position.w;
	
	return o;
}
