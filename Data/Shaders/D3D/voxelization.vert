#include "include/ubos.hlsli"
//#include "include/utils.glsl

cbuffer MaterialData : register(b1)
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

struct GeomInput
{
	float4 position : SV_POSITION;
	float2 uv : TEXCOORD0;
	float3 worldPos : TEXCOORD1;
	float3 normal : TEXCOORD2;
	float4 lightSpacePos : TEXCOORD3;
};

GeomInput VS(VertexInput i)
{
	GeomInput o;
	o.uv = i.uv;
	
	float4 wPos;
	
	float4 pos = float4(i.pos, 1.0);
	float4 N = float4(i.normal, 0.0);
	
#ifdef INSTANCING
	float4x4 modelMatrix = {
		i.modelMCol1.x, i.modelMCol1.y, i.modelMCol1.z, i.modelMCol1.w,
		i.modelMCol2.x, i.modelMCol2.y, i.modelMCol2.z, i.modelMCol2.w,
		i.modelMCol3.x, i.modelMCol3.y, i.modelMCol3.z, i.modelMCol3.w,
		i.modelMCol4.x, i.modelMCol4.y, i.modelMCol4.z, i.modelMCol4.w
	};

	o.normal = mul(N, modelMatrix).xyz;
	wPos = mul(pos, modelMatrix);
#else	
	float4x4 toWorldSpace = GetModelMatrix(startIndex);
	o.normal = mul(N, toWorldSpace).xyz;
	wPos = mul(pos, toWorldSpace);
#endif
	
	o.worldPos = wPos.xyz;
	o.position = wPos;
	
	o.lightSpacePos = mul(wPos, lightSpaceMatrix[0]);
	o.lightSpacePos.xyz = o.lightSpacePos.xyz / o.lightSpacePos.w;
	o.lightSpacePos.xy = o.lightSpacePos.xy * 0.5 + 0.5;		// Only multiply the xy because the z is already in [0,1] (set with glClipControl)
	
	return o;
}
