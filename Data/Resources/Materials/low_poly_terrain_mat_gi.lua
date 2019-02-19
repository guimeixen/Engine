low_poly_terrain_mat_gi = 
{
	passes =
	{
		vctgi =
		{
			vertex="voxelization_terrain",
			geometry="voxelization_terrain",
			fragment="voxelization_terrain",
			enableColorWriting=false,
			depthTest=false,
			blending=false,
			cullface="none",
		},
		base =
		{
			shader="terrain_low_poly"
		},
	},
	materialUBO =
	{
		terrainParams="vec2",
		selectionPointAndRadius="vec3"
	},
	resources =
	{
		heightmap =
		{
			resType="texture2D",
			uv="clamp",
			texFormat="red",
			storeData=true
		},
		colorMap =
		{
			resType="texture2D",
			filter="nearest",
			texFormat='rgba',
			storeData=true
		},
		roughnessMap = 
		{
			resType='texture2D',
			texFormat='red',
		},
	}
}