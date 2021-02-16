#version 450
#extension GL_GOOGLE_include_directive : enable
#include "include/ubos.glsl"

layout(location = 0) out vec4 color;

layout(location = 0) in vec2 uv;

//tex2D_g(CSM_TEXTURE) shadowMap;
//tex2DShadow_g(CSM_TEXTURE) shadowMap;
tex2D_u(0) tex;

PROPERTIES
{
	int isShadowMap;
};

float linDepth(float depth)
{
	float zN = 2.0 * depth - 1.0;
	float zE = 2.0 * nearFarPlane.x * nearFarPlane.y / (nearFarPlane.y + nearFarPlane.x - zN * (nearFarPlane.y - nearFarPlane.x));

	return zE / nearFarPlane.y;	// Divide by zFar to distribute the depth value over the whole viewing distacne
}

void main()
{
	if (isShadowMap > 0)
		color.rgb = texture(tex, uv).rrr;
	else
		color.rgb = texture(tex, uv).rgb;
	/*else if (isVoxelTexture)
		color.rgb = imageLoad(voxelTexture, );
	else
		color.rgb = texture(tex, uv).rgb;*/
	
	//vec2 nUV = uv;
	//nUV.x = nUV.x * ONE_OVER_CASCADE_COUNT;
	//float depth = texture(shadowMap, nUV).r;
	//float linearDepth = linDepth(depth);
	//color.rgb = vec3(depth);
	//color.rgb = texture(shadowMap, uv).rrr;
	//color.rgb = vec3(1.0, 0.0, 0.0);
	color.a = 1.0;
}