#include "include/ubos.hlsli"

Texture2D reflectionTex : register(index0);
SamplerState reflectionTexSampler : register(s1);

Texture2D normalMap : register(index1);
SamplerState normalMapSampler : register(s2);

Texture2D refractionTex : register(index2);
SamplerState refractionTexSampler : register(s3);

Texture2D refractionDepthTex : register(index3);
SamplerState refractionDepthTexSampler : register(s4);

Texture2D foamTexture : register(index4);
SamplerState foamTextureSampler : register(s5);

struct PixelInput
{
	float4 position : SV_POSITION;
	float3 worldPos : TEXCOORD0;
	float4 clipSpacePos : TEXCOORD1;
};

static const float twoPI = 2 * 3.14159;
static const float wavelength[4] = { 13.0, 9.9, 7.3, 6.0 };
static const float amplitude[4]  = { 0.2, 0.12, 0.08, 0.02 };
static const float speed[4] = { 3.4, 2.8, 1.8, 0.6 };
static const float2 dir[4] = { float2(1.0, -0.2), float2(1.0, 0.6), float2(-0.2, 1.0), float2(-0.43, -0.8) };

float LinearizeDepth(float depth)
{
	return 2.0 * nearFarPlane.x * nearFarPlane.y / (nearFarPlane.y + nearFarPlane.x - (2.0 * depth - 1.0) * (nearFarPlane.y - nearFarPlane.x));
}

float3 waveNormal(float2 pos)
{
	float x = 0.0;
	float z = 0.0;
	float y = 0.0;
		
    for (int i = 0; i < 4; ++i)
	{
		float frequency = twoPI / wavelength[i];
		float phase = speed[i] * frequency;
		float q = 0.98 / (frequency * amplitude[i] * 4);
		
		float DdotPos = dot(dir[i], pos);
		float c = cos(frequency * DdotPos + phase * timeElapsed);
		float s = sin(frequency * DdotPos + phase * timeElapsed);
	
		float term2 = frequency * amplitude[i] * c;
		x += dir[i].x * term2;
		z += dir[i].y * term2;
		y += q * frequency * amplitude[i] * s;
	}

    return normalize(float3(-x, 1 - y, -z));
}

float4 PS(PixelInput i) : SV_TARGET
{
	float3 N = waveNormal(i.worldPos.xz);
	float3 V = normalize(camPos.xyz - i.worldPos);
	float3 H = normalize(V + dirAndIntensity.xyz);
	
	float2 tex1 = (i.worldPos.xz * 0.055 + waterNormalMapOffset.xy);
	float2 tex2 = (i.worldPos.xz * 0.02 + waterNormalMapOffset.zw);

	float3 normal1 = normalMap.Sample(normalMapSampler, tex1).rbg * 2.0 - 1.0;
	float3 normal2 = normalMap.Sample(normalMapSampler, tex2).rbg * 2.0 - 1.0;
	float3 fineNormal = normal1 + normal2;

	float detailFalloff = saturate((i.clipSpacePos.w - 60.0) / 40.0);
    fineNormal = normalize(lerp(fineNormal, float3(0.0, 2.0, 0.0), saturate(detailFalloff - 8.0)));
    N = normalize(lerp(N, float3(0.0, 1.0, 0.0), detailFalloff));
	
	// Transform fine normal to world space
    float3 tangent = cross(N, float3(0.0, 0.0, 1.0));
    float3 bitangent = cross(tangent, N);
   // N = tangent * fineNormal.x + N * fineNormal.y + bitangent * fineNormal.z;
	N += fineNormal;
	N = normalize(N);
	
	float2 ndc = i.clipSpacePos.xy / i.clipSpacePos.w * 0.5 + 0.5;
	float2 reflectionUV = float2(ndc.x, ndc.y);
	float2 refractionUV = ndc;
	
	float2 offset = N.xz * float2(0.05, -0.05);
	reflectionUV += offset;
	refractionUV += offset;
	
	reflectionUV = clamp(reflectionUV, 0.001, 0.999);
	refractionUV = clamp(refractionUV, 0.001, 0.999);
	
	float3 reflection = reflectionTex.Sample(reflectionTexSampler, reflectionUV).rgb;
	float3 refraction = refractionTex.Sample(refractionTexSampler, refractionUV).rgb;
	
	
	float3 shallowColor = float3(0.3, 1.0, 0.8);
	float3 deepColor = float3(0.11, 0.28, 0.54);
	
	float depth = refractionDepthTex.Sample(refractionDepthTexSampler, refractionUV).r;
	float waterBottomDistance = LinearizeDepth(depth);
	depth = i.position.z;
	float waterTopDistance = LinearizeDepth(depth);
	float waterDepth = waterBottomDistance - waterTopDistance;
	
	float3 col = lerp(shallowColor, deepColor, saturate(waterDepth * 0.1));
	refraction = lerp(refraction, col, saturate(waterDepth * 0.3));
	
	// Fresnel Effect
	float NdotV = max(dot(N, V), 0.0);
	float fresnel = pow(1.0 - NdotV, 5.0) * 0.5;
	fresnel = saturate(fresnel);

	float3 specular = pow(max(dot(N, H), 0.0), 256.0) * dirLightColor.xyz;
	
	float3 foam = foamTexture.Sample(foamTextureSampler, i.worldPos.xz * 0.05).rgb;

	float4 outColor = (0.0).xxxx;
	outColor.rgb = lerp(refraction, reflection, fresnel);
	outColor.rgb += specular * 3.0;
	//outColor.rgb = lerp(foam, outColor.rgb, saturate(waterDepth * 0.09));
	outColor.rgb *= dirAndIntensity.w;

	outColor.a = saturate(waterDepth * 0.4);
	
	return outColor;
}