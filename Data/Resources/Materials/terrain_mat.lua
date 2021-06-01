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
		[0] =
		{
			name="heightmap",
			resType='texture2D',
			uv='clamp',
			texFormat='r16',
			storeData=true,
			useMipMaps=false,
			usedAsStorageInCompute=true,
		},
		[1] =
		{
			name="diffuseR",
			resType="texture2D"
		},
		[2] =
		{	
			name="diffuseG",
			resType="texture2D"
		},
		[3] =
		{
			name="diffuseB",
			resType="texture2D"
		},
		[4] =
		{
			name="diffuseBlack",
			resType="texture2D"
		},
		[5] =
		{
			name="diffuseRNormal",
			resType="texture2D",
		},
		[6] =
		{
			name="splatmap",
			resType="texture2D",
		}
	}
}