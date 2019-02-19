#version 450

layout(location = 0) out vec4 outColor;
layout(location = 1) out vec4 outNormal;

in vec2 uv;
in float morph;
in vec3 worldPos;
flat in vec3 normal;
in vec3 smoothNormal;
flat in vec3 color;
in vec4 lightSpacePos[4];
in float clipSpaceDepth;
in vec3 tangent;
in vec3 bitangent;

#include include/ubos.glsl
#include include/shadow.glsl
#include include/voxel_cone_tracing.glsl

layout(std140, binding = 1) uniform MaterialUBO
{
	vec2 terrainParams;					// x - resolution, y - height scale
	vec3 selectionPointRadius;			// xy - selection point, z - radius
};

void main()
{
	vec3 lighting = vec3(0.0);
	
	vec3 N = normalize(normal);
	vec3 smoothN = normalize(smoothNormal);
	
	outNormal.rgb = N;
	outNormal.a = 1.0;

	outColor.rgb = color;

	// Directional light
	float NdotL = dot(N, dirAndIntensity.xyz);

	float shadow = calcShadow(clipSpaceDepth, NdotL);

	vec3 diff = max(NdotL, 0.0) * dirLightColor * dirAndIntensity.w;
	lighting += diff * shadow + skyColor * skyColorMultiplier;
	
	// Point lights
	for (int i = 0; i < currentPointLights; i++)
	{
		vec3 wPosToLight = pointLights[i].posAndIntensity.xyz - worldPos;
		vec3 pLightDir = normalize(wPosToLight);
		vec3 pDiff = max(dot(N, pLightDir), 0.0) * pointLights[i].colorAndRadius.xyz * pointLights[i].posAndIntensity.w;

		float dist = length(wPosToLight);
		float att = clamp(1.0 - dist / pointLights[i].colorAndRadius.w, 0.0, 1.0);
		att *= att;
		pDiff *= att;

		lighting += pDiff;
	}
	
	vec4 indirectDiffuse = vec4(0.0);
	//if (all(greaterThan(worldPos, vec3(-8.0 + camPos.x, -8.0 + camPos.y, -8.0 + camPos.z))) && all(lessThan(worldPos, vec3(8.0 + camPos.x, 8.0 + camPos.y, 8.0 + camPos.z))))
	//{
		//indirectDiffuse = indirectDiffuseLight(smoothN);
	//}
	
	if (enableGI > 0)
	{
		indirectDiffuse = indirectDiffuseLight(smoothN);
		lighting += indirectDiffuse.rgb;
		lighting += skyColor * skyColorMultiplier;
		lighting *= indirectDiffuse.a;
	}
	
	outColor.rgb *= lighting;
	
	//outColor.rgb = vec3( indirectDiffuse.rgb);
	
	//outColor.rgb=outNormal;
	//outColor.rgb = mix(outColor.rgb, projectedTexel.rgb, projectedTexel.a);
	outColor.a = 1.0;
	
	/*int cascadeIndex = 0;

	if(clipSpaceDepth > cascadeEnd.z)
		cascadeIndex = 3;
	else if(clipSpaceDepth > cascadeEnd.y)
		cascadeIndex = 2;
	else if(clipSpaceDepth > cascadeEnd.x)
		cascadeIndex = 1;
	
	outColor.rgb = vec3(shadow) * cascadeColor[cascadeIndex];*/
	
	// Heightmap display
	//float c = texture(heightmap, uv).r / 32.0;
	//outColor = vec4(outColor.r + c, c, c, 1.0);
}