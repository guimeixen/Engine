#version 450
#extension GL_GOOGLE_include_directive : enable
#include "include/ubos.glsl"

layout(location = 0) in vec2 uv;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec4 lightSpacePos;
layout(location = 3) in vec3 worldPos;
layout(location = 4) flat in int axis;

tex2D_u(0) texDiffuse;
tex2DShadow_g(CSM_TEXTURE) shadowMap;
image3D_g(VOXEL_IMAGE, rgba8, coherent volatile) voxelTexture;


//layout(set = 1, binding = 0) uniform sampler2D texDiffuse;
//layout(set = 0, binding = 6, rgba8) uniform coherent volatile image3D voxelTexture;
//layout(set = 0, binding = 4) uniform sampler2DShadow shadowMap;


//#include "include/voxelization_helpers.glsl"

/*vec4 convRGBA8ToVec4(uint val)
{
	return vec4(float((val & 0x000000FF)),
					   float((val & 0x0000FF00) >> 8U),
					   float((val & 0x00FF0000) >> 16U),
					   float((val & 0xFF000000) >> 24U));
}

uint convVec4ToRGBA8( vec4 val)
{
	return (uint(val.w) & 0x000000FF) << 24U |
			   (uint(val.z) & 0x000000FF) << 16U |
			   (uint(val.y) & 0x000000FF) << 8U |
			   (uint(val.x) & 0x000000FF);
}

// From OpenGL Insights 
// In Vulkan glsl, we cannot use layout qualifiers as function parameters
//void imageAtomicRGBA8Avg(layout (r32ui) coherent volatile uimage3D voxelGrid, ivec3 coords , vec4 value)
void imageAtomicRGBA8Avg(ivec3 coords, vec4 value)
{
	value.rgb *= 255.0;			// Optimise following calculations
	uint newVal = convVec4ToRGBA8(value);
	uint prevStoredVal = 0;
	uint curStoredVal;
	const int maxIterations = 50;
	int i = 0;
	// Loop as long as destination value gets changed by other threads
	while ((curStoredVal = imageAtomicCompSwap(voxelTexture, coords, prevStoredVal, newVal)) != prevStoredVal && i < maxIterations)
	{
		prevStoredVal = curStoredVal;
		vec4 rval = convRGBA8ToVec4(curStoredVal);
		rval.rgb = (rval.rgb * rval.a);			// Denormalize
		vec4 curValF = rval + value; 			// Add new value
		curValF.rgb /= (curValF.a); 				// Renormalize
		newVal = convVec4ToRGBA8(curValF);
		++i;
	}
}*/


void main()
{
	ivec3 dim = imageSize(voxelTexture);
	vec3 tempPos = vec3(gl_FragCoord.xy, gl_FragCoord.z * dim.z);
	vec3 voxelPos;
	
	if (axis == 1)
	{
		voxelPos.x = dim.x - tempPos.z;
		voxelPos.z = dim.x - tempPos.x;			// Flip the z otherwise the scene is flipped on the z axis
		voxelPos.y = tempPos.y;
	}
	else if (axis == 2)
	{
		voxelPos.z = dim.y - tempPos.y;
		voxelPos.y = dim.y - tempPos.z;		// Flip the z
		voxelPos.x = tempPos.x;
	}
	else
	{
		voxelPos.z = dim.z - tempPos.z;
		voxelPos.x = tempPos.x;
		voxelPos.y = tempPos.y;
	}
	
	vec3 N = normalize(normal);
	
	/*vec3 shadowUV = lightSpacePos.xyz;
	shadowUV.y = 1.0 - shadowUV.y;
	shadowUV.x = shadowUV.x * ONE_OVER_CASCADE_COUNT;
	float bias = 0.00065;
	shadowUV.z -= bias;
	float shadow = texture(shadowMap, shadowUV).r;*/
	
	vec3 diffuse = max(dot(N, dirAndIntensity.xyz), 0.0) * dirLightColor.xyz * dirAndIntensity.w;
	
	//vec3 lighting = diffuse * shadow;
	vec3 lighting = diffuse;
	
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
	
/*#ifdef ALPHA
	vec3 color = vec3(0.01, 0.793, 0.696) * 2.5;
#else
	vec3 color = lighting * texture(texDiffuse, uv).rgb/* + vec3(1.0,0.0,0.0)//;
#endif*/

	vec3 color = lighting * texture(texDiffuse, uv).rgb;

	//imageAtomicRGBA8Avg(voxelTexture, ivec3(voxelPos), vec4(color,1.0));
	//imageAtomicRGBA8Avg(ivec3(voxelPos), vec4(color, 1.0));
	
	imageStore(voxelTexture, ivec3(voxelPos), vec4(color, 1.0));
}