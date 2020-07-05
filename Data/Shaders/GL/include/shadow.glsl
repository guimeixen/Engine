#include "../../common.glsl"

tex2DShadow_g(CSM_TEXTURE) shadowMap;
//layout(binding = SHADOW_MAP_SLOT) uniform sampler2DShadow shadowMap;

const vec3 cascadeColor[4] = vec3[](vec3(1.0, 0.0, 0.0), vec3(0.0, 1.0, 0.0), vec3(0.0, 0.0, 1.0), vec3(1.0, 1.0, 0.0));

const vec2 poissonDisk[8] = vec2[](
	vec2(0.5713538, 0.7814451),
    vec2(0.2306823, 0.6228884),
    vec2(0.1000122, 0.9680607),
    vec2(0.947788, 0.2773731),
    vec2(0.2837818, 0.303393),
    vec2(0.6001099, 0.4147638),
    vec2(-0.2314563, 0.5434746),
    vec2(-0.08173513, 0.0796717)
);

float calcShadow(float clipSpaceDepth, float NdotL)
{
	// Surfaces that are facing away from the light don't receive light so they're in shadow. This fixes some artifacts.
	if (NdotL < 0.0)
		return 0.0;

	int cascadeIndex = 0;

	if(clipSpaceDepth > cascadeEnd.z)
		cascadeIndex = 3;
	else if(clipSpaceDepth > cascadeEnd.y)
		cascadeIndex = 2;
	else if(clipSpaceDepth > cascadeEnd.x)
		cascadeIndex = 1;

	vec3 projCoords = lightSpacePos[cascadeIndex].xyz;

	if (projCoords.z > 1.0)
		return 1.0;
	
	// This fragment/pixel light space position is in NDC [-1,1]. To use it as uv transform it to [0,1]
	// Just transform the xy, the z is already in [0,1] because we set the range with glClipControl
	projCoords.xy = projCoords.xy * 0.5 + 0.5;

	vec3 uv = projCoords.xyz;
	uv.x = uv.x * ONE_OVER_CASCADE_COUNT + ONE_OVER_CASCADE_COUNT * cascadeIndex;

	//float bias = max(0.002 * (1.0 - NdotL), 0.00065);
	float bias = 0.0007;
	
	//float currentDepth = projCoords.z;			// Depth of the current fragment/pixel from the light's perspective
	uv.z -= bias;
	//float shadow = texture(shadowMap, uv).r;
	
	//float shadow = currentDepth - bias > closestDepth ? 0.0 : 1.0;		// If currentDepth is higher than closestDepth than the fragment/pixel is in shadow

	vec2 pixelSize = 1.0 / textureSize(shadowMap, 0);
	float shadow = 0.0;
	vec3 tempUV;
    for(int x = -1; x <= 1; x++)
	{
        for(int y = -1; y <= 1; y++)
		{
			tempUV = uv;
			tempUV.xy += vec2(x,y) * pixelSize;
            shadow += texture(shadowMap, tempUV).r;
        }
    }
	shadow /= 9.0;
	
	/*float sum = 0;
	float x, y;
	for (y = -1.5; y <= 1.5; y += 1.0)
		for (x = -1.5; x <= 1.5; x += 1.0)
	*/		
	
	/*float sampleDiscSize = 0.5;
	float pixelSize = ONE_OVER_SHADOW_MAP_RES * sampleDiscSize;

	for (int i = 0; i < 8; i++)
	{
		float depth = texture(shadowMap, uv + poissonDisk[i] * pixelSize).r;

		if (depth > currentDepth - bias)
		{
			shadow += 1.0;
		}
	}

	shadow /= 8;*/

	return shadow;
}
