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
	--objectUBO = 
	--{
	--	params="vec2",
	--},
	resources =
	{
		reflectionTexture =
		{
			resType="texture2D"
		},
		normalMap =
		{
			resType="texture2D"
		},
		refractionTexture =
		{
			resType="texture2D"
		},
		refractionDepthTexture =
		{
			resType="texture2D"
		},
		foamTexture =
		{
			resType="texture2D"
		},
	}
}