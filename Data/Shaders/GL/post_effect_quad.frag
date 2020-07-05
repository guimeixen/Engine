#version 450
#include "include/ubos.glsl"

layout(location = 0) out vec4 color;

layout(location = 0) in vec2 uv;

tex2D_u(0) sceneHDR;
tex2D_u(1) sceneDepth;
tex2D_u(2) bloomTex;
tex2D_u(3) cloudsTexture;

/*layout(binding = FIRST_SLOT) uniform sampler2D sceneHDR;
layout(binding = FIRST_SLOT + 1) uniform sampler2D sceneDepth;
layout(binding = FIRST_SLOT + 2) uniform sampler2D bloomTex;
layout(binding = FIRST_SLOT + 3) uniform sampler2D cloudsTexture;*/

const int samples = 65;

// Returns the world position given the non-linearized depth value
vec3 worldPosition(float depth)
{
	//float zN = 2.0 * depth - 1.0;				// No need to convert from [0,1] to [-1,1] because we're using glClipControl to change it to [0,1] instead of the default [-1,1]
	vec4 clipSpacePos = vec4(uv * 2.0 - 1.0, depth, 1.0);
	vec4 viewSpacePos = invProj * clipSpacePos;
	viewSpacePos /= viewSpacePos.w;
	vec4 worldSpacePos = invView * viewSpacePos;

	return worldSpacePos.xyz;
}

vec3 applyFog(vec3 originalColor, vec3 V, vec3 worldPos)
{	
	// Lengyel Unified fog
	// We assume a plane with normal (0,1,0)
	vec3 aV = fogParams.y * 0.5 * V;
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
	float fogAmount = clamp(exp2(-g), 0.0, 1.0);

	V = normalize(V);
	float sunAmount = max(dot(V, -dirAndIntensity.xyz), 0.0);
	vec3 fogColor = mix(fogInscatteringColor.rgb, lightInscatteringColor.rgb, sunAmount * sunAmount * sunAmount);
	
	return mix(fogColor, originalColor, fogAmount);
}

float linDepth(float depth)
{
	float zN = 2.0 * depth - 1.0;
	float zE = 2.0 * nearFarPlane.x * nearFarPlane.y / (nearFarPlane.y + nearFarPlane.x - zN * (nearFarPlane.y - nearFarPlane.x));

	return zE / nearFarPlane.y;	// Divide by zFar to distribute the depth value over the whole viewing distacne
}

vec3 upsample(sampler2D tex)
{
	vec2 texelSize = 1.0 / textureSize(tex, 0); 	// gets size of single texel
    
	vec4 offset = texelSize.xyxy * vec4(-1.0, -1.0, 1.0, 1.0) * (1.0 * 0.5);
	vec3 result = texture(tex, uv + offset.xy).rgb;
	result += texture(tex, uv + offset.zy).rgb;
	result += texture(tex, uv + offset.xw).rgb;
	result += texture(tex, uv + offset.zw).rgb;
	
	result *= 0.25;

	return result;
}

vec3 lightShafts()
{
	vec2 tex = uv;

	vec2 deltaTextCoord = vec2(uv - lightScreenPos);
	deltaTextCoord *= 1.0 / float(samples) * lightShaftsParams.x;
	float illuminationDecay = 1.0;

	float blurred = 0.0;

	for (int i = 0; i < samples; i++)
	{
		tex -= deltaTextCoord;
		//float depth = texture(sceneDepth, tex).r;
		float curSample = /*linDepth(depth) + */(1.0 - texture(cloudsTexture, tex).a);
		curSample = smoothstep( 0.7, 1.0, curSample);

		curSample *= illuminationDecay * lightShaftsParams.z;
		blurred += curSample;
		illuminationDecay *= lightShaftsParams.y;
	}

	blurred *= lightShaftsParams.w;
	blurred *= smoothstep(1.0, 0.0, distance(uv, lightScreenPos));

	return vec3(blurred);
}

void main()
{
	color = texture(sceneHDR, uv);
	vec3 bloom = upsample(bloomTex) * bloomIntensity;
	float depth = texture(sceneDepth, uv).r;
	float linearDepth = linDepth(depth);
	
	vec3 worldPos = worldPosition(depth);
	vec3 worldPosToCam = camPos.xyz - worldPos;
	
	float a = 1.0;
	if (linearDepth > 0.99)
	{
		vec4 clouds = texture(cloudsTexture, uv);
		color.rgb = color.rgb * (1.0 - clouds.a) + clouds.rgb;
		a = 1.0 - clouds.a;
	}

	color.rgb = applyFog(color.rgb, worldPosToCam, worldPos);
	//vec3 blurredScene = lightShafts() * lightShaftsColor.rgb * lightShaftsIntensity;
	vec3 blurredScene = lightShafts() * dirLightColor.rgb * lightShaftsIntensity;
	color.rgb += blurredScene;
	color.rgb += bloom * a;

	color.rgb = pow(color.rgb, vec3(0.45));
	
	// Vignette
	// Length from the center
	// Multiply x by the aspect ratio
	vec2 coord = (uv - vec2(0.5)) * vec2(screenRes.x / screenRes.y, 1.0) * vignetteParams.x;
	float len = length(coord);
	len *= vignetteParams.y;
	float vignette = len * len + 1.0;
	vignette = 1.0 / (vignette * vignette);

	color.rgb = color.rgb * vignette;
	
	color.a  = 1.0;
}