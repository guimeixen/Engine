gizmo_mat = 
{
	passes = 
	{
		base =
		{
			queue='opaque',
			shader="debug",
			--depthTest=false,	--Don't use depth test false otherwise it doesn't render because of the skydome
			depthFunc='always'
		}
	},
}