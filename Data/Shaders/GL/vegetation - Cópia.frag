#version 450

layout(location = 0) out vec4 outColor;
layout(location = 1) out vec4 outNormal;

in vec2 uv;
in vec3 normal;
in vec3 worldPos;
in vec4 lightSpacePos[3];
in float clipSpaceDepth;

layout(binding = 0) uniform sampler2D tex;

#include include/ubos.glsl
#include include/shadow.glsl

const vec3 matDiffuseColor = vec3(1.0);
const vec3 matSpecularColor = vec3(0.57);
const vec3 backDiffuseColor = vec3(0.7, 0.8, 0.37);
const float backViewDependency = 0.65;
const float backDiffuseMultiplier = 4.0;
const float shininess = 105.0;

const vec3 hemiTop = vec3(0.0, 1.0, 0.0);
const vec3 topHemiColor = vec3(0.47, 0.91, 0.93);
const vec3 bottomHemiColor = vec3(0.51, 0.83, 0.19);

vec3 LeafShadingBack(vec3 N, vec3 L, vec3 V)
{
	float VdotL = clamp(dot(V, -L), 0.0, 1.0);
	VdotL *= VdotL;
	float NdotLBack = clamp(dot(-N, L), 0.0, 1.0);
	vec3 backShading = vec3(mix(VdotL, NdotLBack, backViewDependency));
	
	return backShading * backDiffuseColor * backDiffuseMultiplier;
}

void LeafShadingFront(vec3 N, vec3 L, vec3 H, vec3 diffuseColor, vec3 specColor, out vec3 diffuse, out vec3 specular)
{
	float NdotL = clamp(dot(N, L), 0.0, 1.0);
	diffuse = NdotL * diffuseColor;
	specular = pow(clamp(dot(N, H), 0.0, 1.0), shininess) * specColor;
}

void main()
{
	vec3 N = normalize(normal);
	outNormal.rgb = N;
	outNormal.a = 1.0;

	vec4 diffuseMap = texture(tex, uv);
	
	if (diffuseMap.a < 0.35)
		discard;
		
	vec3 lighting = vec3(0.0);
	
	vec3 V = normalize(camPos.xyz - worldPos);
	vec3 H = normalize(dirAndIntensity.xyz + V);

	float NdotL = dot(N, dirAndIntensity.xyz);

	float shadow = calcShadow(clipSpaceDepth, NdotL);
	vec3 diff = max(NdotL, 0.0) * dirLightColor * dirAndIntensity.w;
	//lighting += (diff * shadow + vec3(0.03));
	//vec3 spec = pow(max(dot(N, H), 0.0), 32.0) * dirLightColor;
	//lighting += (diff + vec3(ambient) + spec);
	//lighting += shadow + vec3(ambient) * dirLightColor * dirAndIntensity.w;
	
	float ndotl = dot(N, hemiTop);
	float lightInfluence = ndotl * 0.5 + 0.5;
	vec3 amb = mix(bottomHemiColor, topHemiColor, lightInfluence);
	amb *= 0.2;
		
	if (gl_FrontFacing)
	{
		vec3 diffuse;
		vec3 specular;
		LeafShadingFront(N, dirAndIntensity.xyz, H, dirLightColor, dirLightColor, diffuse, specular);
		
		lighting = (diffuse * shadow + amb) * diffuseMap.rgb * matDiffuseColor;
		lighting += specular * shadow /** sssValue*/ * matSpecularColor;
		
		// Multiply light color
	}
	else
	{
		vec3 backShading = LeafShadingBack(N, dirAndIntensity.xyz, V);
		//backShading *= sssValue;
		backShading *= shadow;
		
		lighting = (amb + backShading) * diffuseMap.rgb;
	}
	
	outColor.rgb = lighting;

	// Point lights
	/*for (int i = 0; i < currentPointLights; i++)
	{
		vec3 wPosToLight = pointLights[i].posAndIntensity.xyz - worldPos;
		vec3 pLightDir = normalize(wPosToLight);
		vec3 pDiff = max(dot(N, pLightDir), 0.0) * pointLights[i].colorAndRadius.xyz * pointLights[i].posAndIntensity.w;

		float dist = length(wPosToLight);
		float att = clamp(1.0 - dist / pointLights[i].colorAndRadius.w, 0.0, 1.0);
		att *= att;
		pDiff *= att;

		lighting += pDiff;
	}*/
	
	//outColor.rgb = N;
	outColor.a = 1.0;
}