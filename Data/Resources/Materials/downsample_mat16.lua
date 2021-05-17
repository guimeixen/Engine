downsample_mat16 = 
{
	passes = 
	{
		DownsamplePass16 =
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