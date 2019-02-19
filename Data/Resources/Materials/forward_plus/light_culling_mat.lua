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
		depth =
		{
			resType="texture2D",
		},
	}
}