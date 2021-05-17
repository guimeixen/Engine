fxaa_mat = 
{
	passes = 
	{
		fxaa =
		{
			vertex='quad',
			fragment='fxaa',
		}
	},
	resources =
	{
		[0] =
		{
			name="postProcessedTexture",
			resType="texture2D"
		}
	}
}