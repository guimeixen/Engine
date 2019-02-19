aurora_mat = 
{
	passes =
	{
		--reflection = 
		--{
		--	shader="model_reflection"
		--},
		alpha =
		{
			shader="aurora",
			cullface="none",
			blending=true,
			srcBlendColor="src_alpha",
			srcBlendAlpha="src_alpha",
			dstBlendColor="one",
			dstBlendAlpha="one",
			depthWrite=false
		}	
	},
	resources =
	{
		diffuse =
		{
			resType="texture2D"
		}
	}
}