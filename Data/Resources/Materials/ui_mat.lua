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
		[0] =
		{
			name="texture",
			resType="texture2D",
			useMipmaps=false
		}
	}
}