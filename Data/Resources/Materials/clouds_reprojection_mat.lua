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
		cloudLowResTexture =
		{
			resType="texture3D"
		},
		previousFrameTexture =
		{
			resType="texture3D"
		},
	}
}