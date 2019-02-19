#include "include/ubos.hlsli"

struct VertexInput
{
	float4 posUV : POSITION;
	float4 params : NORMAL;			// xy - translation, z - size, w - range
};

struct PixelInput
{
	float4 position : SV_POSITION;
	float2 uv : TEXCOORD0;
	float3 worldPos : TEXCOORD1;
	float3 normal : TEXCOORD2;
	float clipSpaceDepth : TEXCOORD3;
	float4 lightSpacePos[3] : TEXCOORD4;
};

Texture2D heightmap : register(index0);
SamplerState heightmapSampler : register(s1);


static const float2 gridDim = float2(16.0, 16.0);
static const float2 size = float2(2.0, 0.0);
static const int3 off = int3(-1, 0, 1);

// gridPos - position in the terrain in the range [0,1], we can use the uv
// worldPos - world space position of the vertex to morph
// morphK - morph value

float2 morphVertex(float2 gridPos, float2 worldPos, float morphK, float size)
{
	float2 fracPart = frac(gridPos * gridDim * 0.5) * 2.0 / gridDim;

	return worldPos - fracPart * size * morphK;
}

PixelInput VS(VertexInput i)
{
	PixelInput o;
	o.uv = i.posUV.zw;
	//float texelSize = 1.0 / terrainParams.x;
	float texelSize = 1.0 / 256.0;
	float2 newUv = o.uv * texelSize * i.params.z + i.params.xy * texelSize;
	
	float h = heightmap.SampleLevel(heightmapSampler, newUv, 0).r * 256.0;
	o.worldPos = float3(o.uv.x * i.params.z + i.params.x, h, o.uv.y * i.params.z + i.params.y);

	float dist = distance(camPos.xyz, o.worldPos);
	float start = i.params.w * 0.7;	// We start morphing after 70%
	float morphConstZ = i.params.w / (i.params.w - start);
	float morphConstW = 1.0 / (i.params.w - start);
	float morphK = 1.0 - clamp(morphConstZ - dist * morphConstW, 0.0, 1.0);

	o.worldPos.xz = morphVertex(o.uv, o.worldPos.xz, morphK, i.params.z);

	newUv = o.worldPos.xz * texelSize;
	o.uv = newUv;
	h = heightmap.SampleLevel(heightmapSampler, newUv, 0).r * 256.0;

	// Normal calculation
	float left = heightmap.SampleLevel(heightmapSampler, o.uv, 0, off.xy).x * 256.0;
    float right = heightmap.SampleLevel(heightmapSampler, o.uv, 0, off.zy).x * 256.0;
    float down = heightmap.SampleLevel(heightmapSampler, o.uv, 0, off.yx).x * 256.0;
    float up = heightmap.SampleLevel(heightmapSampler, o.uv, 0, off.yz).x * 256.0;
	o.normal = normalize(float3(left - right, 2.0, down - up));
	
	o.worldPos.y = (h + left + right + up + down) * 0.2;

	o.position = mul(float4(o.worldPos, 1.0), projView);
	
	o.lightSpacePos[0] = mul(float4(o.worldPos, 1.0), lightSpaceMatrix[0]);
	o.lightSpacePos[1] = mul(float4(o.worldPos, 1.0), lightSpaceMatrix[1]);
	o.lightSpacePos[2] = mul(float4(o.worldPos, 1.0), lightSpaceMatrix[2]);

	o.clipSpaceDepth = o.position.w / nearFarPlane.y;
	
	return o;
}
