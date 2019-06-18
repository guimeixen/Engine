model_inst_mat = 
{
	passes =
	{
		csm = 
		{
			shader="shadow_map"
		},
		base = 
		{
			queue='opaque',
			shader="model",
		},
		depthPrepass =
		{
			shader='depth_prepass',
		}
	},
	resources =
	{
		diffuse =
		{
			resType="texture2D"
		}
	}
}