terrain_low_poly_mat = 
{
	passes =
	{
		base =
		{
			queue='opaque',
			shader='terrain_low_poly'
		},
		depthPrepass =
		{
			shader='depth_prepass',
		}
	},
	resources =
	{
		heightmap =
		{
			resType='texture2D',
			uv='clamp',
			--texFormat='r16',
			texFormat='red',
			storeData=true,
			useMipMaps=false,
			usedAsStorageInCompute=true,
		},
		colorMap =
		{
			resType='texture2D',
			--filter="nearest",
			texFormat='rgba',
			storeData=true
		},
	}
}