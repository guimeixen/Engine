#include "include/common.hlsli"

cbuffer CameraCB : register(b0)
{
	float4x4 proj;
	float4x4 view;
	float4x4 projView;
	float4x4 invView;
	float4x4 invProj;
	float4 clipPlane;
	float4 camPos;
	float2 nearFarPlane;
};

cbuffer FrameCB : register(b3)
{
	float4x4 orthoProjX;
	float4x4 orthoProjY;
	float4x4 orthoProjZ;
	float4x4 previousFrameView;
	float4x4 cloudsInvProjJitter;
	float4x4 projGridViewFrame;
	float4 viewCorner0;
	float4 viewCorner1;
	float4 viewCorner2;
	float4 viewCorner3;
	float4 waterNormalMapOffset;
	float timeElapsed;
	float giIntensity;
	float skyColorMultiplier;
	float aoIntensity;
	float voxelGridSize;
	float voxelScale;
	int enableGI;
	float timeOfDay;
	float bloomIntensity;
	float bloomThreshold;
	float2 windDirStr;
	float4 lightShaftsParams;
	float4 lightShaftsColor;
	float2 lightScreenPos;
	float2 fogParams;				// x - fog height, y - fog density
	
	//float bloomRadius;
	//float sampleScale;
	float4 fogInscatteringColor;
	float4 lightInscatteringColor;
	
	//vec4
	float cloudCoverage;
	float cloudStartHeight;
	float cloudLayerThickness;
	float cloudLayerTopHeight;
	
	//vec4
	float timeScale;
	float hgForward;
	float densityMult;
	float ambientMult;
	
	//vec4
	float directLightMult;
	float detailScale;
	float highCloudsCoverage;
	float highCloudsTimeScale;
	
	//vec4
	float silverLiningIntensity;
	float silverLiningSpread;
	float forwardSilverLiningIntensity;
	float lightShaftsIntensity;
	
	float4 ambientTopColor;
	float4 ambientBottomColor;
	
	float2 screenRes;
	//float2 invScreenRes;
	float2 vignetteParams;
	
	uint frameNumber;
	uint cloudUpdateBlockSize;
};

cbuffer DirLight : register(b4)
{
	float4x4 lightSpaceMatrix[4];
	float4 dirAndIntensity;			// xyz - direction, w - intensity
	float4 dirLightColor;				// xyz - color, w - ambient
	float4 cascadeEnd;
	float3 skyColor;
};

struct PointLight
{
	float4 posAndIntensity;			// xyz - position, w - intensity
	float4 colorAndRadius;			// xyz - color, w - radius
};

cbuffer DispatchParams : register(b2)
{
	uint3 numWorkGroups;
	float pad0;
};

cbuffer PointLights : register(b5)
{
	PointLight pointLights[MAX_POINT_LIGHTS];
	int currentPointLights;
};

StructuredBuffer<float4x4> instanceData : register(INSTANCE_DATA_BINDING);

float4x4 GetModelMatrix(uint index)
{
	//vec4 m0 = instanceData[startIndex];
	//vec4 m1 = instanceData[startIndex+1];
	//vec4 m2 = instanceData[startIndex+2];
	//vec4 m3 = instanceData[startIndex+3];
	//mat4 toWorldSpace = mat4(1.0);
	//toWorldSpace[0] = m0;
	//toWorldSpace[1] = m1;
	//toWorldSpace[2] = m2;
	//toWorldSpace[3] = m3;
	return instanceData[index];
}