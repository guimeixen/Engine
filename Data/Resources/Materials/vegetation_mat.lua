vegetation_mat = 
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
			shader="vegetation",
			cullface="none"
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
			name="diffuse",
			resType="texture2D"
		}
	}
}