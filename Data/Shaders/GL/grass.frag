#version 450
#include include/ubos.glsl
#include include/shadow.glsl

layout(location = 0) out vec4 outColor;
layout(location = 1) out vec4 outNormal;

in vec2 uv;
in vec3 normal;
in vec3 worldPos;
in vec4 lightSpacePos[3];
in float clipSpaceDepth;

layout(binding = FIRST_SLOT) uniform sampler2D tex;

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
	vec3 frontLightDif;
	vec3 frontLightSpec;
	
	lighting += diff * shadow + vec3(ambient);
	
//	vec3 diffuseColor = vec3(0.56, 0.58, 0.64);
	
	//LeafShadingFront(V, L, N, diffuseColor, vec4(0.0), frontLightDif, frontLightSpec);

	
	
	//lighting += frontLightDif;
	
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
	
	//vec3 final = ((vec3(ambient) + diff) * diffuseColor + shadingBack) * diffuseMap.rgb;

	outColor.rgb *= lighting;
	//outColor.rgb = final;
	outColor.a = 1.0;
}