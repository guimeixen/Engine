model_mat = 
{
	passes =
	{
		csm = 
		{
			queue='csm',
			shader="shadow_map",
			blending=false,
			srcBlendColor='zero',
			dstBlendColor='zero',
			srcBlendAlpha='zero',
			dstBlendAlpha='zero',
			enableColorWriting=false,		-- Disable color writing so libgxm disables the fragment shader
			cullface='front'							-- Cull the front faces
		},
		base =
		{
			queue='opaque',
			shader="model",
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