light_culling_mat =
{
	passes =
	{
		lightCulling =
		{
			computeShader="light_cull",
		}
	},
	resources =
	{
		[0] =
		{
			name="depth",
			resType="texture2D",
		},
	}
}