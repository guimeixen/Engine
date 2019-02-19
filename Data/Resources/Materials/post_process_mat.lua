post_process_mat = 
{
	passes = 
	{
		postProcessPass =
		{
			shader="post_effect_quad",
			frontFace="ccw",
			cullface="back"
		}
	},
	resources =
	{
		hdrTexture =
		{
			resType="texture2D"
		},
		depthTexture =
		{
			resType="texture2D"
		},
		bloomTexture =
		{
			resType="texture2D"
		},
		normalsTexture =
		{
			resType="texture2D"
		},
		computeTexture =
		{
			resType="texture2D"
		},
	}
}