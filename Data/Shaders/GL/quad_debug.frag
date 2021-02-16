#version 450
#include "include/ubos.glsl"

layout(location = 0) out vec4 color;

layout (location = 0) in vec2 uv;

tex2D_g(CSM_TEXTURE) shadowMap;

//layout(binding = CSM_TEXTURE) uniform sampler2D tex;
/*layout(binding = 1) uniform sampler3D voxelTexture;

layout(std140, binding = OBJECT_UBO_BINDING) uniform ObjectUBO
{
	int isShadowMap;
	int isVoxelTexture;
	float voxelZSlice;
	int mipLevel;
	int voxelRes;
};*/

void main()
{
	/*if (isShadowMap > 0)
		color.rgb = texture(tex, uv).rrr;
	else if (isVoxelTexture > 0)
		color.rgb = texelFetch(voxelTexture, ivec3(uv * voxelRes, voxelZSlice), mipLevel).rgb;
	else
		color.rgb = texture(tex, uv).rgb;
		*/
		color.rgb = texture(shadowMap, uv).rrr;
		//color.rgb=vec3(0.0,0.0,1.0);
	color.a = 1.0;
}