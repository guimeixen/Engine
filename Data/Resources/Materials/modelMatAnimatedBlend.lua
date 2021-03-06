modelMatAnimatedBlend = 
{
	passes =
	{
		alpha =
		{
			shader="model",
			blending=true,
			srcBlendColor="src_alpha",
			srcBlendAlpha="src_alpha",
			dstBlendColor="one_minus_src_alpha",
			dstBlendAlpha="one_minus_src_alpha",
			--depthWrite=false
		}	
	},
	objectUBO = 
	{
		boneTransforms="mat4[32]"
	},
	resources =
	{
		diffuse =
		{
			resType="texture2D",
			texFormat="rgba",
			useAlpha=true
		}
	}
}