low_poly_water_mat = 
{
	passes = 
	{
		base =
		{
			shader="low_poly_water",
			frontFace="ccw",
			cullFace="back",
			blending=true,
		}
	},
	objectUBO = 
	{
		params="float",
	},
	resources =
	{
		reflectionTexture =
		{
			resType="texture2D"
		},
	}
}