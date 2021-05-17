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
		[0] =
		{
			name="transmittanceTexture",
			resType="texture2D",
		},
		[1] = 
		{
			name="inscatterTexture",
			resType="texture3D",
		}
	}
}