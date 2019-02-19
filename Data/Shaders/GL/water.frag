#version 450

layout(location = 0) out vec4 outColor;

in vec2 uv;
in vec4 clipSpacePos;
in vec3 worldPos;

layout(binding = 0) uniform sampler2D reflectionTex;
layout(binding = 1) uniform sampler2D normalMap;
layout(binding = 2) uniform sampler2D dudvMap;

#include include/ubos.glsl

layout(std140, binding = OBJECT_UBO_BINDING) uniform ObjectUBO
{
	vec2 params;			// x - moveFactor, y - waveStrength
};

void main()
{
	vec3 V = normalize(camPos.xyz - worldPos);
	vec3 H = normalize(V + dirAndIntensity.xyz);
	
	vec2 ndc = clipSpacePos.xy / clipSpacePos.w * 0.5 + 0.5;
	vec2 reflectionUV = vec2(ndc.x, 1.0 - ndc.y);
	//vec2 refractTexCoords = ndc;
	
	vec2 distortedTexCoords = texture(dudvMap, vec2(uv.x + params.x, uv.y)).rg * 0.1;	
	distortedTexCoords = uv + vec2(distortedTexCoords.x, distortedTexCoords.y + params.x);
	vec2 totalDistortion = (texture(dudvMap, distortedTexCoords).rg * 2.0 - 1.0) * params.y;
	
	reflectionUV += totalDistortion;
	reflectionUV = clamp(reflectionUV, 0.001, 0.999);
	vec3 reflection = texture(reflectionTex, reflectionUV).rgb;
	
	vec3 N = texture(normalMap, distortedTexCoords).rbg * 2.0 - 1.0;	// Normal.b correponds to the up axis in tangent space
	N = normalize(N);
	
	// Transform fine normal to world space
   /* vec3 tangent = cross(N, vec3(0.0, 0.0, 1.0));
    vec3 bitangent = cross(tangent, N);
    N = tangent * N.x + N * N.y + bitangent * N.z;
	N = normalize(N);*/
	
	// Fresnel
/*	float NdotV = dot(V, N);
	float fresnel = pow(1.0 - NdotV, 5.0);
	// Clamp otherwise some black pixelated artifacts will show
	float reflectionAmount = 8.0;
	fresnel = clamp(fresnel * reflectionAmount, 0.0, 1.0);*/
	
	vec3 specular = pow(max(dot(N, H), 0.0), 256.0) * dirLightColor * 4.0;
	
	outColor.rgb = reflection;
	//outColor.rgb *= 0.2;
	outColor.rgb += specular;
	//outColor.rgb = N;
	outColor.a = 1.0;
}