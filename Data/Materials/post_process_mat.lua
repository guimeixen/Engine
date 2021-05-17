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
		[0] =
		{
			name="hdrTexture",
			resType="texture2D"
		},
		[1] =
		{
			name="depthTexture",
			resType="texture2D"
		},
		[2] =
		{
			name="bloomTexture",
			resType="texture2D"
		},
		[3] =
		{
			name="normalsTexture",
			resType="texture2D"
		},
		[4] =
		{
			name="computeTexture",
			resType="texture2D"
		},
	}
}