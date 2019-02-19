#include "include/ubos.hlsli"

struct VertexInput
{
	float2 inUV : POSITION;
};

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

float3 wave(float2 pos)
{
	float3 wave = float3(0.0,0.0,0.0);
		
    for (int i = 0; i < 4; ++i)
	{
		float frequency = twoPI / wavelength[i];
		float phase = speed[i] * frequency;
		float q = 0.98 / (frequency * amplitude[i] * 4);
		
		float DdotPos = dot(dir[i], pos);
		float c = cos(frequency * DdotPos + phase * timeElapsed);
		float s = sin(frequency * DdotPos + phase * timeElapsed);
		
		float term = q * amplitude[i] * c;
		
		wave.x += term * dir[i].x;
		wave.z += term * dir[i].y;
		wave.y += amplitude[i] * s;
	}

    return wave;
}

PixelInput VS(VertexInput i)
{
	PixelInput o;
	
	float4 pos = lerp(lerp(viewCorner1, viewCorner0, i.inUV.x), lerp(viewCorner2, viewCorner3, i.inUV.x), i.inUV.y);
	pos.xyz /= pos.w;
	o.worldPos = pos.xyz;
	
	float l = length(camPos.xyz - o.worldPos);
	o.worldPos += wave(o.worldPos.xz) *  (1.0 - smoothstep(55.0, 100.0, l));
	o.clipSpacePos = mul(float4(o.worldPos, 1.0), projView);
	o.position = o.clipSpacePos;
	
	return o;
}
