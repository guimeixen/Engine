#include "include/ubos.hlsli"

struct VertexInput
{
	float4 posUv : POSITION;
};

struct PixelInput
{
	float4 position : SV_POSITION;
	float2 uv : TEXCOORD0;
	float3 camRay : TEXCOORD1;
};

float3 UVToCamRay(float2 uv)
{
	float4 clipSpacePos = float4(uv * 2.0 - 1.0, 1.0, 1.0);
	float4 viewSpacePos = mul(clipSpacePos, transpose(cloudsInvProjJitter));
	viewSpacePos.xyz /= viewSpacePos.w;
	float4 worldSpacePos = mul(viewSpacePos, invView);
	
	return worldSpacePos.xyz;
}

PixelInput VS(VertexInput i)
{
	PixelInput o;

	o.position = float4(i.posUv.xy, 0.1, 1.0);
	o.uv =float2(i.posUv.z, 1.0-i.posUv.w);
	o.camRay = UVToCamRay(o.uv);

	return o;
}
