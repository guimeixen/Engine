skydome_mat = 
{
	passes = 
	{
		base =
		{
			shader="skydome",
			frontFace="ccw",
			cullface="front",
			depthFunc="lequal",
			--topology='lines'
		}
	},
	resources =
	{
		transmittanceTexture =
		{
			resType="texture2D",
		},
		inscatterTexture = 
		{
			resType="texture3D",
		}
	}
}