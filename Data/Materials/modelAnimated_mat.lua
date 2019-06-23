modelAnimated_mat = 
{
	passes =
	{
		--csm = 
		--{
		--	shader="shadow_map"
		--},
		base =
		{
			queue='opaque',
			shader='model'
		}
	},
	objectUBO = 
	{
		boneTransforms="mat4[32]"
	},
	resources =
	{
		diffuse =
		{
			resType="texture2D"
		}
	}
}