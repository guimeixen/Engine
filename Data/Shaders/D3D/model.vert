#include "include/ubos.hlsli"

cbuffer MaterialData : register(b1)
{
	uint startIndex;
};

struct VertexInput
{
	float3 pos : POSITION;
	float2 uv : NORMAL;
	float3 normal : TEXCOORD;
#ifdef ANIMATED
	int4 boneIDs : TANGENT;
	float4 weights : BINORMAL;
#endif
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
	float clipSpaceDepth : TEXCOORD3;
	float3 lightSpacePos[3] : TEXCOORD4;
};

PixelInput VS(VertexInput i)
{
	PixelInput o;
	
	o.uv = i.uv;
	float4 wPos;
	
#ifdef ANIMATED
	uint index = startIndex + 1;

	float4x4 boneTransform = mul(i.weights[0], instanceData[index + i.boneIDs[0]]);
	boneTransform += mul(i.weights[1], instanceData[index + i.boneIDs[1]]);
	boneTransform += mul(i.weights[2], instanceData[index + i.boneIDs[2]]);
	boneTransform += mul(i.weights[3], instanceData[index + i.boneIDs[3]]);
	boneTransform = transpose(boneTransform);

	float4 pos = mul(float4(i.pos, 1.0), boneTransform);		// Transform from bone space to local space
	float4 N = mul(float4(i.normal, 0.0), boneTransform);
#else
	float4 pos = float4(i.pos, 1.0);
	float4 N = float4(i.normal, 0.0);
#endif
	
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
	o.position = mul(wPos, projView);
	
	o.lightSpacePos[0] = mul(wPos, lightSpaceMatrix[0]);
	o.lightSpacePos[1] = mul(wPos, lightSpaceMatrix[1]);
	o.lightSpacePos[2] = mul(wPos, lightSpaceMatrix[2]);
	
	o.clipSpaceDepth = o.position.w / nearFarPlane.y;
	
	return o;
}