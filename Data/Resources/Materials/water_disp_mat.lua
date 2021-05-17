water_disp_mat = 
{
	passes = 
	{
		base =
		{
			shader="water_disp",
			frontFace="ccw",
			cullFace="back",
			blending=true,
		}
	},
	resources =
	{
		[0] =
		{
			name="reflectionTexture",
			resType="texture2D"
		},
		[1] =
		{
			name="normalMap",
			resType="texture2D"
		},
		[2] =
		{
			name="refractionTexture",
			resType="texture2D"
		},
		[3] =
		{
			name="refractionDepthTexture",
			resType="texture2D"
		},
		[4] =
		{
			name="foamTexture",
			resType="texture2D"
		},
	}
}