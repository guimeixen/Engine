voxel_cube_vis_mat =
{
	passes =
	{
		base =
		{
			--queue='opaque',
			shader="voxels_cube_vis",
			depthTest=true,
			cullface="back"
		}
	},
}