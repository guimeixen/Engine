clouds_reprojection_mat = 
{
	passes = 
	{
		cloudsReprojectionPass =
		{
			vertex='quad',
			fragment='cloud_reprojection',
			frontFace='ccw',
			cullface='back'
		}
	},
	resources =
	{
		[0] =
		{
			name="cloudLowResTexture",
			resType="texture2D"
		},
		[1] =
		{
			name="previousFrameTexture",
			resType="texture2D"
		},
	}
}