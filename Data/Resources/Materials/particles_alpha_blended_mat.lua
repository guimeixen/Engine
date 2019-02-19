particles_alpha_blended_mat = 
{
	passes =
	{
		alpha = 
		{
			shader="particles",
			blending=true,
			srcBlendColor="src_alpha",
			srcBlendAlpha="src_alpha",
			dstBlendColor="one_minus_src_alpha",
			dstBlendAlpha="one_minus_src_alpha",
			depthWrite=false
		}
	},
	materialUBO =
	{
		params="vec4"
	},
	resources =
	{
		diffuse =
		{
			resType="texture2D"
		}
	}
}