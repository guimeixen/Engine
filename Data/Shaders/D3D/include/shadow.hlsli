#include "include/common.hlsli"

Texture2D shadowMap : register(CSM_BINDING);
SamplerComparisonState shadowMapSampler : register(s0);

float calcShadow(float clipSpaceDepth, float NdotL, float3 lightSpacePos[3])
{
	// Surfaces that are facing away from the light don't receive light so they're in shadow. This fixes some artifacts	(dot prod -1 means the normal is facing way from the light)
	if (NdotL < 0.0)
		return 0.0;

	int cascadeIndex = 0;

	if(clipSpaceDepth > cascadeEnd.z)
		cascadeIndex = 3;
	else if(clipSpaceDepth > cascadeEnd.y)
		cascadeIndex = 2;
	else if(clipSpaceDepth > cascadeEnd.x)
		cascadeIndex = 1;

	float3 projCoords = lightSpacePos[cascadeIndex];

	if (projCoords.z > 1.0)
		return 1.0;
	
	// This fragment/pixel light space position is in NDC [-1,1]. To use it as uv transform it to [0,1]
	// Just transform the xy, the z is already in [0,1] because we set the range with glClipControl
	projCoords.xy = projCoords.xy * 0.5 + 0.5;

	float3 uv = projCoords.xyz;
	uv.y = 1.0 - uv.y;		// Flip the y because Vulkan is top left instead of bottom left like OpenGL
	uv.x = uv.x * ONE_OVER_CASCADE_COUNT + ONE_OVER_CASCADE_COUNT * cascadeIndex;

	//float bias = max(0.002 * (1.0 - NdotL), 0.00065);
	float bias = 0.0007;
	
	//float currentDepth = projCoords.z;			// Depth of the current fragment/pixel from the light's perspective
	uv.z -= bias;
	//float shadow = texture(shadowMap, uv).r;
	
	//float shadow = currentDepth - bias > closestDepth ? 0.0 : 1.0;		// If currentDepth is higher than closestDepth than the fragment/pixel is in shadow

	float2 dim;
	shadowMap.GetDimensions(dim.x, dim.y);
	float2 pixelSize = 1.0 / dim;
	
	float shadow = 0.0;
    for(int x = -1; x <= 1; x++)
	{
        for(int y = -1; y <= 1; y++)
		{
            float2 off = float2(x,y) * pixelSize;
			uv.xy += off;
            shadow += shadowMap.SampleCmp(shadowMapSampler, uv.xy, uv.z).r;
        }
    }
	shadow /= 9.0;

	return shadow;
}