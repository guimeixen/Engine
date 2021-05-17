downsample_mat8 = 
{
	passes = 
	{
		DownsamplePass8 =
		{
			shader="downsample"
		}
	},
	resources =
	{
		[0] =
		{
			name="downsampleTexture",
			resType="texture2D"
		}
	}
}