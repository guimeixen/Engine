model_inst_mat_gi = 
{
	passes =
	{
		vctgi =
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
			shader="model",
			cullface="none"
		}	
	},
	resources =
	{
		[0] =
		{
			name="diffuse",
			resType="texture2D"
		}
	}
}