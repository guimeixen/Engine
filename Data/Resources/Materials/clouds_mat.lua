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
		[0] =
		{
			name="baseNoiseTexture",
			resType="texture3D"
		},
		[1] =
		{
			name="highFreqNoiseTexture",
			resType="texture3D"
		},
		[2] =
		{
			name="weatherTexture",
			resType="texture2D"
		},
	}
}