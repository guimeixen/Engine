#include "include/ubos.hlsli"

Texture2D cloudLowResTexture : register(index0);
SamplerState cloudLowResSampler : register(s1);

Texture2D previousFrameTexture : register(index1);
SamplerState previousFrameSampler : register(s2);

struct PixelInput
{
	float4 position : SV_POSITION;
	float2 uv : TEXCOORD0;
};

float4 PS(PixelInput i) : SV_TARGET
{
	// We need to check if the current pixel was rendered to the quarter res buffer
	// If so we use that value for the pixel because it's the most up to date value
	// Otherwise we reproject the pixel using the result from the previous frame if it is inside the screen
	// and if it is outside we use the result from the up to date low res quarter buffer
	
	float2 prevTexDim;
	previousFrameTexture.GetDimensions(prevTexDim.x, prevTexDim.y);
	float2 scaledUV = floor(i.uv * prevTexDim);
	
	float2 cloudLowResDim;
	cloudLowResTexture.GetDimensions(cloudLowResDim.x, cloudLowResDim.y);
	float2 uv2 = (floor(i.uv * cloudLowResDim) + 0.5) / cloudLowResDim;

	float x = fmod(scaledUV.x, cloudUpdateBlockSize);
	float y = fmod(scaledUV.y, cloudUpdateBlockSize);
	uint frame = y * cloudUpdateBlockSize + x;
	
	if (frame == frameNumber)
	{
		//return float4(0.0, 0.0, 1.0, 1.0);
		return cloudLowResTexture.Sample(cloudLowResSampler, uv2);
	}
	else
	{
		float4 worldPos = float4(i.uv * 2.0 - 1.0, 1.0, 1.0);
		worldPos = mul(worldPos, invProj);
		worldPos /= worldPos.w;
		worldPos = mul(worldPos, invView);
	
		float4 prevFramePos = mul(float4(worldPos.xyz, 1.0), transpose(previousFrameView));
		prevFramePos = mul(prevFramePos, proj);
		prevFramePos.xy /= prevFramePos.w;
		prevFramePos.xy = prevFramePos.xy * 0.5 + 0.5;
		prevFramePos.y = 1.0 - prevFramePos.y;
		
		//bool isOut = any(greaterThan(abs(prevFramePos.xy - vec2(0.5)), vec2(0.5)));
		if (prevFramePos.x < 0.0 || prevFramePos.x > 1.0 || prevFramePos.y < 0.0 || prevFramePos.y > 1.0)
			//return float4(1.0,0.0,0.0,1.0);
			return cloudLowResTexture.Sample(cloudLowResSampler, i.uv);
		else
		//	return float4(0.0, 1.0, 0.0, 1.0);
			return previousFrameTexture.Sample(previousFrameSampler, prevFramePos.xy);
	}
}
