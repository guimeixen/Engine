upsample_mat = 
{
	passes = 
	{
		Upsample8 =
		{
			shader="upsample"
		},
		Upsample4 =
		{
			shader="upsample"
		},
	},
	resources =
	{
		[0] =
		{
			name="upsampleTexture",
			resType="texture2D"
		},
		[1] =
		{
			name="baseTexture",
			resType="texture2D"
		},
	}
}