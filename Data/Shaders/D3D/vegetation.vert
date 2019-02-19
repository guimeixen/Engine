#include "include/ubos.hlsli"

cbuffer MaterialData : register(b3)
{
	uint startIndex;
};

struct VertexInput
{
	float3 pos : POSITION;
	float2 uv : NORMAL;
	float3 normal : TEXCOORD;
#ifdef INSTANCING
	float4 modelMCol1 : TANGENT;
	float4 modelMCol2 : BINORMAL;
	float4 modelMCol3 : COLOR;
	float4 modelMCol4 : FOG;
#endif
};

struct PixelInput
{
	float4 position : SV_POSITION;
	float2 uv : TEXCOORD0;
	float3 worldPos : TEXCOORD1;
	float3 normal : TEXCOORD2;
};

PixelInput VS(VertexInput i)
{
	PixelInput o;
	o.uv = i.uv;
	float4 wPos;
	
#ifdef INSTANCING
	float4x4 modelMatrix = {
		i.modelMCol1.x, i.modelMCol1.y, i.modelMCol1.z, i.modelMCol1.w,
		i.modelMCol2.x, i.modelMCol2.y, i.modelMCol2.z, i.modelMCol2.w,
		i.modelMCol3.x, i.modelMCol3.y, i.modelMCol3.z, i.modelMCol3.w,
		i.modelMCol4.x, i.modelMCol4.y, i.modelMCol4.z, i.modelMCol4.w
	};

	o.normal  = mul(float4(i.normal, 0.0), modelMatrix).xyz;
	wPos = mul(float4(i.pos, 1.0), modelMatrix);
#else	
	float4x4 toWorldSpace = GetModelMatrix(startIndex);
	o.normal  = mul(float4(i.normal, 0.0), toWorldSpace).xyz;
	wPos = mul(float4(i.pos, 1.0), toWorldSpace);
#endif
	
	o.worldPos = wPos.xyz;
	o.position = mul(wPos, projView);
	
	return o;
}