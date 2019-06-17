terrain_mat = 
{
	passes =
	{
		base =
		{
			queue='opaque',
			shader='terrain'
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
			texFormat='red',
			storeData=true,
			useMipMaps=false,
			usedAsStorageInCompute=true,
		},
		diffuseR =
		{
			resType="texture2D"
		},
		diffuseG =
		{
			resType="texture2D"
		},
		diffuseB =
		{
			resType="texture2D"
		},
		diffuseBlack =
		{
			resType="texture2D"
		},
		diffuseRNormal =
		{
			resType="texture2D",
			texFormat="rgb"
		},
		splatmap =
		{
			resType="texture2D",
		}
	}
}