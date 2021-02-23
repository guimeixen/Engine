model_mat_gi = 
{
	passes =
	{
		voxelization =
		{
			vertex="voxelization",
			geometry="voxelization",
			fragment="voxelization",
			enableColorWriting=false,
			depthTest=false,
			blending=false,
			cullface="none",
		},
		csm = 
		{
			shader="shadow_map"
		},
		base =
		{
			queue='opaque',
			shader="model",
		},
		--depthPrepass =
		--{
		--	shader='depth_prepass',
		--}
	},
	resources =
	{
		diffuse =
		{
			resType="texture2D"
		}
	}
}