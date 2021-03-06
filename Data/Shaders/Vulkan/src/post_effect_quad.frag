#version 450
#extension GL_GOOGLE_include_directive : enable
#include "include/ubos.glsl"

layout(location = 0) out vec4 color;

layout(location = 0) in vec2 uv;

tex2D_u(0) sceneHDR;
tex2D_u(1) sceneDepth;
tex2D_u(2) bloomTex;
tex2D_u(3) cloudsTexture;
tex2D_u(4) computeImg;

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
	
	//vec4 F = vec4(0.0, 1.0, 0.0, 0.0);
	//float FdotP = dot(F, vec4(worldPos, 1.0));
	//float FdotC = dot(F, vec4(camPos, 1.0));
	//float FdotV = dot(F, vec4(pointToCam, 0.0));

	float c1 = k * (FdotP + FdotC);
	float c2 = (1.0 - 2.0 * k) * FdotP;
	
	float g = min(c2, 0.0);
	g = -length(aV) * (c1 - g * g / abs(FdotV + 1.0e-5));
	float fogAmount = clamp(exp2(-g), 0.0, 1.0);
	
	/*fogAmount *= length(worldPos);
	fogAmount = clamp(fogAmount, 0.0, 1.0);*/

	V = normalize(V);
	float sunAmount = max(dot(V, -dirAndIntensity.xyz), 0.0);
	vec3 fogColor = mix(fogInscatteringColor.rgb, lightInscatteringColor.rgb, sunAmount * sunAmount * sunAmount);
	
	return mix(fogColor, originalColor, fogAmount);
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

float linDepth(float depth)
{
	float zN = 2.0 * depth - 1.0;
	float zE = 2.0 * nearFarPlane.x * nearFarPlane.y / (nearFarPlane.y + nearFarPlane.x - zN * (nearFarPlane.y - nearFarPlane.x));

	return zE / nearFarPlane.y;	// Divide by zFar to distribute the depth value over the whole viewing distacne
}

vec3 lightShafts()
{
	vec2 tex = uv;
	
	vec2 deltaTextCoord = vec2(uv - vec2(lightScreenPos.x, 1.0 - lightScreenPos.y));			// Flip the y because of the Vulkan ndc
	deltaTextCoord *= 1.0 / float(samples) * lightShaftsParams.x;
	float illuminationDecay = 1.0;

	float blurred = 0.0;

	for (int i = 0; i < samples; i++)
	{
		tex -= deltaTextCoord;
		float depth = texture(sceneDepth, tex).r;
		float curSample =  (1.0 - texture(cloudsTexture, tex).a) * linDepth(depth);

		curSample *= illuminationDecay * lightShaftsParams.z;
		blurred += curSample;
		illuminationDecay *= lightShaftsParams.y;
	}

	blurred *= lightShaftsParams.w;
	//float d = distance(uv, vec2(lightScreenPos.x, 1.0 - lightScreenPos.y));
	//blurred *= smoothstep(1.0, 0.0, distance(uv, lightScreenPos));
	//blurred *= smoothstep(1.0, 0.0, d * d);

	return vec3(blurred);
}

void main()
{
	color.rgb = texture(sceneHDR, uv).rgb;
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
	
	//float t = 0.5 - 1.4 * 0.5; 
    //color.rgb = color.rgb * 1.4 + t;
	
	color.a = 1.0;
}