particles_additive_mat = 
{
	passes =
	{
		base = 
		{
			queue='transparent',
			shader="particles",
			blending=true,
			srcBlendColor="src_alpha",
			srcBlendAlpha="src_alpha",
			dstBlendColor="one",
			dstBlendAlpha="one",
			depthWrite=false
		}
	},
	materialUBO =
	{
		params="vec4"
	},
	resources =
	{
		[0] =
		{
			name="diffuse",
			resType="texture2D"
		}
	}
}