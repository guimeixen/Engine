#version 450

layout(location = 0) out vec4 color;

layout(location = 1) in vec2 uv;

//#include include/ubos.glsl

//layout(set = 0, binding = 6, rgba8) uniform image3D voxelTexture;
layout(set = 1, binding = 0) uniform sampler2D tex;

layout(push_constant) uniform PushConsts
{
	int isShadowMap;
	//int isVoxelTexture;
};

void main()
{
	if (isShadowMap > 0)
		color.rgb = texture(tex, uv).rrr;
	/*else if (isVoxelTexture)
		color.rgb = imageLoad(voxelTexture, );*/
	else
		color.rgb = texture(tex, uv).rgb;
	color.a = 1.0;
}