vec4 convRGBA8ToVec4(uint val)
{
	return vec4(float((val & 0x000000FF)),
					   float((val & 0x0000FF00) >> 8U),
					   float((val & 0x00FF0000) >> 16U),
					   float((val & 0xFF000000) >> 24U));
}

uint convVec4ToRGBA8( vec4 val)
{
	return (uint(val.w) & 0x000000FF) << 24U |
			   (uint(val.z) & 0x000000FF) << 16U |
			   (uint(val.y) & 0x000000FF) << 8U |
			   (uint(val.x) & 0x000000FF);
}

// From OpenGL Insights 
void imageAtomicRGBA8Avg(layout (r32ui) coherent volatile uimage3D voxelGrid, ivec3 coords , vec4 value)
{
	value.rgb *= 255.0;			// Optimise following calculations
	uint newVal = convVec4ToRGBA8(value);
	uint prevStoredVal = 0;
	uint curStoredVal;
	const int maxIterations = 50;
	int i = 0;
	// Loop as long as destination value gets changed by other threads
	while ((curStoredVal = imageAtomicCompSwap(voxelGrid, coords, prevStoredVal, newVal)) != prevStoredVal && i < maxIterations)
	{
		prevStoredVal = curStoredVal;
		vec4 rval = convRGBA8ToVec4(curStoredVal);
		rval.rgb = (rval.rgb * rval.a);			// Denormalize
		vec4 curValF = rval + value; 			// Add new value
		curValF.rgb /= (curValF.a); 				// Renormalize
		newVal = convVec4ToRGBA8(curValF);
		++i;
	}
}
