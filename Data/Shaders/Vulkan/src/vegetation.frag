#version 450
#extension GL_GOOGLE_include_directive : enable
#include "include/ubos.glsl"

layout(location = 0) out vec4 outColor;

layout(location = 0) in vec2 uv;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec3 worldPos;
layout(location = 3) in float clipSpaceDepth;
layout(location = 4) in vec4 lightSpacePos[3];

layout(set = 1, binding = 0) uniform sampler2D tex;

#include "include/shadow.glsl"

const vec3 matDiffuseColor = vec3(1.0);
const vec3 matSpecularColor = vec3(0.57);
const vec3 backDiffuseColor = vec3(0.7, 0.8, 0.37);
const float backViewDependency = 0.65;
const float backDiffuseMultiplier = 4.0;
const float shininess = 185.0;

const vec3 hemiTop = vec3(0.0, 1.0, 0.0);
const vec3 topHemiColor = vec3(0.47, 0.91, 0.93);
const vec3 bottomHemiColor = vec3(0.51, 0.83, 0.19);

vec3 LeafShadingBack(vec3 N, vec3 L, vec3 V)
{
	float VdotL = clamp(dot(V, -L), 0.0, 1.0);
	VdotL *= VdotL;
	float NdotLBack = clamp(dot(-N, L), 0.0, 1.0);
	vec3 backShading = vec3(mix(VdotL, NdotLBack, backViewDependency));
	
	return backShading /** backDiffuseColor * backDiffuseMultiplier*/ * dirLightColor.rgb;
}

void main()
{
	vec4 diffuseMap = texture(tex, uv);
	
	if (diffuseMap.a < 0.25)
		discard;
		
	outColor.rgb = diffuseMap.rgb;

	vec3 lighting = vec3(0.0);
	vec3 N = normalize(normal);
	vec3 V = normalize(camPos.xyz - worldPos);
	vec3 H = normalize(dirAndIntensity.xyz + V);
	
	/*if (gl_FrontFacing)
	{*/
		float NdotL = dot(N, dirAndIntensity.xyz);
		vec3 diffuse = max(NdotL, 0.0) * dirLightColor.rgb;
		vec3 specular = pow(clamp(dot(N, H), 0.0, 1.0), 256.0) * dirLightColor.rgb;
		
		float shadow = calcShadow(clipSpaceDepth, NdotL);
		
		lighting = (diffuse * shadow) * matDiffuseColor;
		lighting += specular * shadow /** sssValue*/ * matSpecularColor;
	/*}
	else
	{*/
		//NdotL = dot(-N, dirAndIntensity.xyz);
		//float shadow = calcShadow(clipSpaceDepth, NdotL);
	
		vec3 backShading = LeafShadingBack(N, dirAndIntensity.xyz, V);
		//backShading *= sssValue;
		backShading *= shadow;
		
		lighting += backShading * dirLightColor.rgb;
	//}
	
//	lighting += vec3(dirLightColor.w);		// Ambient
	//outColor.rgb *= lighting * dirAndIntensity.w + dirLightColor.w;	
	
	outColor.rgb *= (diffuse * matDiffuseColor + specular * matSpecularColor + backShading * backDiffuseColor * backDiffuseMultiplier) * shadow * dirAndIntensity.w + dirLightColor.w;
	
	//outColor.rgb = N;
	outColor.a = 1.0;
}