ui_mat = 
{
	passes = 
	{
		postProcessPass =
		{
			queue='ui',
			shader="ui",
			--depthWrite=false,
			blending=true,
			frontFace='ccw',
		}
	},
	resources =
	{
		texture =
		{
			resType="texture2D",
			useMipmaps=false
		}
	}
}