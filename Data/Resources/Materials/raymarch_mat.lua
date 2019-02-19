raymarch_mat = 
{
	passes = 
	{
		base =
		{
			shader="raymarch",
			blending=true,
			srcBlendColor="src_alpha",
			srcBlendAlpha="src_alpha",
			dstBlendColor="one_minus_src_alpha",
			dstBlendAlpha="one_minus_src_alpha",
			depthTest=false,
			depthWrite=false,
			depthFunc='always',
		}
	},
	resources =
	{
		noiseTex =
		{
			resType='texture3D'
		}
	}
}