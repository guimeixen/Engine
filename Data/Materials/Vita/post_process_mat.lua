post_process_mat = 
{
	passes = 
	{
		postProcessPass =
		{
			shader="post",
			frontFace="ccw",
			cullface="back"
		}
	},
	resources =
	{
		hdrTexture =
		{
			resType="texture2D"
		}
	}
}