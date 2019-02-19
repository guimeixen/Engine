#include "include/common.hlsli"

Texture2D sceneHDR : register(index0);
SamplerState samp : register( s1 );

struct PixelInput
{
	float4 position : SV_POSITION;
	float2 uv : TEXCOORD0;
};

float brightness(float3 rgb)
{
	return max(rgb.x, max(rgb.y, rgb.z));
}

float4 PS(PixelInput i) : SV_TARGET
{
	float3 sceneColor = sceneHDR.Sample(samp, i.uv).rgb;
	float br = brightness(sceneColor);

     // Under-threshold part: quadratic curve
     /*float rq = clamp(br - _Curve.x, 0.0, _Curve.y);
     rq = _Curve.z * rq * rq;*/

     // Combine and apply the brightness response curve.
	 float f = max(br - 1.6, 0.0);
     sceneColor *= f / max(br, 1e-5);
	
   return float4(sceneColor, 1.0);
}