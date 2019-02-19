default_skybox_mat = 
{
	passes =
	{
		base = 
		{
			shader="skybox",
			frontFace="ccw",
			cullface="front",
			depthFunc="lequal",
			--depthWrite="false"
		}
	},
	resources =
	{
		skybox =
		{
			resType="textureCube",
			uv="clamp_to_edge",
			alpha=false,
			useMipMaps=false
		}
	}
}