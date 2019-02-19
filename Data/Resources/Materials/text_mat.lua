text_mat = 
{
	passes = 
	{
		postProcessPass =
		{
			shader="text",
			depthWrite=false,
			frontFace="ccw",
			cullface="back",
			blending=true,
			--depthFunc='lequal',
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