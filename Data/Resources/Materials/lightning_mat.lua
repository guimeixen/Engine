lightning_mat = 
{
	passes =
	{
		base = 
		{
			queue='transparent',
			shader="lightning",
			blending=true,
			srcBlendColor="src_alpha",
			srcBlendAlpha="src_alpha",
			dstBlendColor="one_minus_src_alpha",
			dstBlendAlpha="one_minus_src_alpha",
			topology='lines',
			depthTest=false
		}
	},
}