low_poly_terrain_mat = 
{
	passes =
	{
		--reflection =
		--{
		--	shader="terrain" -- could be "terrain_simplified" because it's rendered to a smaller render target
		--},
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
	}
}