debug_mat = 
{
	passes = 
	{
		postProcessPass =
		{
			shader="quad_debug",
			frontFace="ccw",
			cullface="back",
		}
	},
	materialUBO =
	{
		scale="float",
		trans="vec2"
	},
	resources =
	{
		[0] =
		{
			name="debugTexture",
			resType="texture2D",
		},
		--debug3DTexture =
		--{
		--	resType="texture3D",
		--}
	}
}