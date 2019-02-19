#include "include/ubos.hlsli"

struct VertexInput
{
	float3 pos : POSITION;
	uint instanceIndex : SV_InstanceID;
};

struct PixelInput
{
	float4 position : SV_POSITION;
	float3 color : NORMAL;
};

struct Voxel
{
	float3 pos;
	float pad;
};

//layout(set = 0, binding = 6) uniform sampler3D voxelTexture;
Texture3D voxelTexture : register(index0);
SamplerState transmittanceSampler : register(s1);

StructuredBuffer<Voxel> voxelsBuffer : register(index1);

/*layout(std140, binding = OBJECT_UBO_BINDING) uniform ObjectUBO
{
	float volumeSize;
	float mipLevel;
};
*/
PixelInput VS(VertexInput i)
{
	PixelInput o;

	float3 gridPos = voxelsBuffer[i.instanceIndex].pos;

	float scale = voxelGridSize / 128.0;
	//float scale = voxelGridSize / volumeSize;		// Grid size = 64, voxel size = 64 then scale = 1
	float halfSize = voxelGridSize * 0.5;
	
	// Snap the camera so we move in voxel sized increments
	float interval = voxelGridSize * 0.125;  // / 8
	float3 newCamPos = round(camPos.xyz / interval) * interval;
	
	float4x4 m = { scale, 0.0f, 0.0f, gridPos.x * scale - halfSize + newCamPos.x,
							0.0f, scale, 0.0f, gridPos.y * scale - halfSize + newCamPos.y,
							0.0f, 0.0f, scale, gridPos.z * scale - halfSize + newCamPos.z,
							0.0f, 0.0f, 0.0f, 1.0f};
	//m[3] = float4(gridPos * scale - halfSize + newCamPos, 1.0);
	
	//color = textureLod(voxelTexture, gridPos / 128.0, 0).rgb;
	//color = textureLod(voxelTexture, gridPos / volumeSize, mipLevel).rgb;
	//color = imageLoad(voxelTexture, vec3(gridPos / 128.0)).rgb;
	o.color = float3(1.0, 0.0, 0.0);
	
	o.position = mul(float4(i.pos, 1.0), m);
	o.position = mul(o.position, projView);
	
	return o;
}