#include "include/ubos.hlsli"
//#include "include/voxelization_helpers.glsl"

struct PixelInput
{
	float4 position : SV_POSITION;
	float2 uv : TEXCOORD0;
	float3 worldPos : TEXCOORD1;
	float3 normal : TEXCOORD2;
	float4 lightSpacePos : TEXCOORD3;
	nointerpolation int axis : TEXCOORD4;
};

Texture2D texDiffuse : register(index0);
SamplerState diffuseSampler : register(s1);

RWTexture3D<float4> voxelTexture : register(u0);

//layout(set = 1, binding = 0) uniform sampler2D texDiffuse;
//layout(set = 0, binding = 6, rgba8) uniform coherent volatile image3D voxelTexture;
//layout(binding = SHADOW_MAP_SLOT) uniform sampler2DShadow shadowMap;

void PS(PixelInput i)
{
	uint3 dim;
	voxelTexture.GetDimensions(dim.x, dim.y, dim.z);
	float3 tempPos = float3(i.position.xy, i.position.z * dim.z);
	float3 voxelPos;
	
	if (i.axis == 1)
	{
		voxelPos.x = dim.x - tempPos.z;
		voxelPos.z = dim.x - tempPos.x;			// Flip the z otherwise the scene is flipped on the z axis
		voxelPos.y = tempPos.y;
	}
	else if (i.axis == 2)
	{
		voxelPos.z = dim.y - tempPos.y;
		voxelPos.y = dim.y - tempPos.z;		// Flip the z
		voxelPos.x = tempPos.x;
	}
	else
	{
		voxelPos.z = dim.z - tempPos.z;
		voxelPos.x = tempPos.x;
		voxelPos.y = tempPos.y;
	}
	
	float3 N = normalize(i.normal);
	
	float3 color = texDiffuse.Sample(diffuseSampler, i.uv).rgb;
	
	//imageAtomicRGBA8Avg(voxelTexture, ivec3(voxelPos), vec4(color,1.0));
	//imageAtomicRGBA8Avg(ivec3(voxelPos), vec4(color, 1.0));
	
	//imageStore(voxelTexture, ivec3(voxelPos), vec4(color, 1.0));
	voxelTexture[voxelPos] = float4(color, 1.0);
}