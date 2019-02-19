compute_test_mat =
{
	passes =
	{
		computeTest =
		{
			computeShader="compute",
		}
	},
	resources =
	{
		outputImg =
		{
			resType="texture2D",
			usedAsStorageInCompute=true,
		},
		
		inputImg =
		{
			resType='texture2D',
			usedAsStorageInCompute=true,	
		},
		
		testBuffer =
		{
			resType='ssbo',
			
		}
	}
}