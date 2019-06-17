terrain_edit_mat =
{
	passes =
	{
		terrainEdit =
		{
			computeShader="terrain_edit",
		}
	},
	resources =
	{
		img =
		{
			resType="texture2D",
			usedAsStorageInCompute=true,
		},
	}
}