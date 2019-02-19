clouds_mat = 
{
	passes = 
	{
		cloudsPass =
		{
			shader="clouds",
			frontFace="ccw",
			cullface="back"
		}
	},
	resources =
	{
		baseNoiseTexture =
		{
			resType="texture3D"
		},
		highFreqNoiseTexture =
		{
			resType="texture3D"
		},
		weatherTexture =
		{
			resType="texture2D"
		},
	}
}