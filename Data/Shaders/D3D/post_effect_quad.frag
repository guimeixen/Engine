#include "include/ubos.hlsli"

Texture2D sceneHDR : register(index0);
SamplerState samp : register(s1);

Texture2D sceneDepth : register(index1);
SamplerState depthSampler : register(s2);

Texture2D bloomTex : register(index2);
SamplerState bloomSampler : register(s3);

Texture2D cloudsTexture : register(index3);
SamplerState cloudsTextureSampler : register(s4);

struct PixelInput
{
	float4 position : SV_POSITION;
	float2 uv : TEXCOORD0;
};

static const int samples = 65;

// Returns the world position given the non-linearized depth value
float3 worldPosition(float depth, float2 uv)
{
	uv.y = 1.0 - uv.y;
	//float zN = 2.0 * depth - 1.0;				// No need to convert from [0,1] to [-1,1] because we're using glClipControl to change it to [0,1] instead of the default [-1,1]
	float4 clipSpacePos = float4(uv * 2.0 - 1.0, depth, 1.0);
	float4 viewSpacePos = mul(clipSpacePos, invProj);
	viewSpacePos /= viewSpacePos.w;
	float4 worldSpacePos = mul(viewSpacePos, invView);

	return worldSpacePos.xyz;
}

float3 applyFog(float3 originalColor, float3 V, float3 worldPos)
{	
	// Lengyel Unified fog
	// We assume a plane with normal (0,1,0)
	float3 aV = fogParams.y * 0.5 * V;
	float FdotC = camPos.y - fogParams.x;		// Dot produts can be simplified to this because of the zeros in the plane normal and the 1.0 * camPos.y which results in the same thing
	float FdotP = worldPos.y - fogParams.x;
	float FdotV = V.y;
	
	float k = 0.0;
	if (FdotC <= 0.0)
		k = 1.0;

	float c1 = k * (FdotP + FdotC);
	float c2 = (1.0 - 2.0 * k) * FdotP;
	
	float g = min(c2, 0.0);
	g = -length(aV) * (c1 - g * g / abs(FdotV + 1.0e-5));
	float fogAmount = saturate(exp2(-g));

	V = normalize(V);
	float sunAmount = max(dot(V, -dirAndIntensity.xyz), 0.0);
	float3 fogColor = lerp(fogInscatteringColor.rgb, lightInscatteringColor.rgb, sunAmount * sunAmount * sunAmount);
	
	return lerp(fogColor, originalColor, fogAmount);
}


float linDepth(float depth)
{
	float zN = 2.0 * depth - 1.0;
	float zE = 2.0 * nearFarPlane.x * nearFarPlane.y / (nearFarPlane.y + nearFarPlane.x - zN * (nearFarPlane.y - nearFarPlane.x));

	return zE / nearFarPlane.y;	// Divide by zFar to distribute the depth value over the whole viewing distacne
}

float3 upsample(Texture2D tex, SamplerState s, float2 uv)
{
	float2 dim = (0.0).xx;
	tex.GetDimensions(dim.x, dim.y);
	float2 texelSize = 1.0 / dim;
    
	float4 offset = texelSize.xyxy * float4(-1.0, -1.0, 1.0, 1.0) * (1.0 * 0.5);
	float3 result = tex.Sample(s, uv + offset.xy).rgb;
	result += tex.Sample(s, uv + offset.zy).rgb;
	result += tex.Sample(s, uv + offset.xw).rgb;
	result += tex.Sample(s, uv + offset.zw).rgb;
	
	result *= 0.25;

	return result;
}

float3 lightShafts(float2 uv)
{
	float2 tex = uv;

	float2 deltaTextCoord = float2(uv - float2(lightScreenPos.x, 1.0 - lightScreenPos.y));
	deltaTextCoord *= 1.0 / float(samples) * lightShaftsParams.x;
	float illuminationDecay = 1.0;

	float blurred = 0.0;

	for (int i = 0; i < samples; i++)
	{
		tex -= deltaTextCoord;
		float depth = sceneDepth.Sample(depthSampler, tex).r;
		float curSample = linDepth(depth);

		curSample *= illuminationDecay * lightShaftsParams.z;
		blurred += curSample;
		illuminationDecay *= lightShaftsParams.y;
	}

	blurred *= lightShaftsParams.w;

	return (blurred).rrr;
}

float4 PS(PixelInput i) : SV_TARGET
{
	float4 color = sceneHDR.Sample(samp, i.uv);
	float3 bloom = upsample(bloomTex, bloomSampler, i.uv) * bloomIntensity;
	float depth = sceneDepth.Sample(depthSampler, i.uv).r;
	float linearDepth = linDepth(depth);
	
	float3 worldPos = worldPosition(depth, i.uv);
	float3 worldPosToCam = camPos.xyz - worldPos;
	
	float a = 1.0;
	if (linearDepth > 0.99)
	{
		float4 clouds = cloudsTexture.Sample(cloudsTextureSampler, i.uv);
		color.rgb = color.rgb * (1.0 - clouds.a) + clouds.rgb;
		a = 1.0 - clouds.a;
	}

	color.rgb = applyFog(color.rgb, worldPosToCam, worldPos);
	float3 blurredScene = lightShafts(i.uv) * lightShaftsColor.rgb * lightShaftsIntensity;
	color.rgb += blurredScene;
	color.rgb += bloom * a;
	//float3 color = (sceneDepth.Sample(depthSampler, i.uv).r).xxx;
	color.rgb = pow(abs(color.rgb), float3(0.45,0.45,0.45));
	color.a = 1.0;
	
	return color;
}