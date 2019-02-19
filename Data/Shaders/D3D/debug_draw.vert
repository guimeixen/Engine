#include "include/ubos.hlsli"

struct VertexInput
{
	float3 pos : POSITION;
	float4 modelMCol1 : NORMAL;
	float4 modelMCol2 : TEXCOORD;
	float4 modelMCol3 : TANGENT;
	float4 modelMCol4 : BINORMAL;
	float3 color : COLOR;
};

struct PixelInput
{
	float4 position : SV_POSITION;
	float3 color : TEXCOORD0;
};

PixelInput VS(VertexInput i)
{
	PixelInput o;
	
	float4x4 modelMatrix = {
		i.modelMCol1.x, i.modelMCol1.y, i.modelMCol1.z, i.modelMCol1.w,
		i.modelMCol2.x, i.modelMCol2.y, i.modelMCol2.z, i.modelMCol2.w,
		i.modelMCol3.x, i.modelMCol3.y, i.modelMCol3.z, i.modelMCol3.w,
		i.modelMCol4.x, i.modelMCol4.y, i.modelMCol4.z, i.modelMCol4.w
	};
	
	o.color = i.color;
	o.position = mul(float4(i.pos, 1.0), modelMatrix);
	o.position = mul(o.position, projView);
	return o;
}