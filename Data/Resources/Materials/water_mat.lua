water_mat = 
{
	passes = 
	{
		base =
		{
			shader="water",
			frontFace="ccw",
			cullFace="back"
		}
	},
	objectUBO = 
	{
		params="vec2",
	},
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
		dudvMap =
		{
			resType="texture2D"
		},
	}
}