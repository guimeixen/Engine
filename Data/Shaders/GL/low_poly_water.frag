#version 450
#include include/ubos.glsl

layout(location = 0) out vec4 outColor;

layout(location = 0) in vec2 uv;
layout(location = 1) in vec4 clipSpacePos;
layout(location = 2) flat in vec3 normal;
layout(location = 3) in vec3 worldPos;
layout(location = 4) flat in vec3 specular;

layout(binding = FIRST_SLOT) uniform sampler2D reflectionTex;

void main()
{
	vec3 N = normalize(normal);
	vec3 V = normalize(camPos.xyz - worldPos);
	//vec3 H = normalize(V + dirAndIntensity.xyz);

	vec2 ndc = clipSpacePos.xy / clipSpacePos.w * 0.5 + 0.5;
	vec2 reflectionUV = vec2(ndc.x, 1.0 - ndc.y);
	
	vec2 offset = N.xz * vec2(0.05, -0.05);
	reflectionUV += offset;
	reflectionUV = clamp(reflectionUV, 0.001, 0.999);
	
	vec3 reflection = texture(reflectionTex, reflectionUV).rgb;

	// Fresnel
	float NdotV = dot(V, N);
	float fresnel = pow(1.0 - NdotV, 5.0);
	// Clamp otherwise some black pixelated artifacts will show
	float reflectionAmount = 3.0;
	fresnel = clamp(fresnel * reflectionAmount, 0.0, 1.0);	
	
	outColor.rgb = mix(vec3(0.0, 0.93, 0.74) * 0.1, reflection, fresnel);
	outColor.rgb += specular;
	outColor.a = mix(0.5, 1.0, fresnel * fresnel);
}