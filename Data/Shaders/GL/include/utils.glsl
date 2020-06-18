/*mat4 GetModelMatrix(uint startIndex)
{
	//vec4 m0 = instanceData[startIndex];
	//vec4 m1 = instanceData[startIndex+1];
	//vec4 m2 = instanceData[startIndex+2];
	//vec4 m3 = instanceData[startIndex+3];
	//mat4 toWorldSpace = mat4(1.0);
	//toWorldSpace[0] = m0;
	//toWorldSpace[1] = m1;
	//toWorldSpace[2] = m2;
	//toWorldSpace[3] = m3;
	return instanceData[startIndex + gl_InstanceIndex];
}*/

#ifdef ANIMATED
mat4 GetBoneTransform(uint startIndex)
{
	uint index = startIndex ;
	mat4 boneTransform = transforms[index + boneIDs[0]] * weights[0];
	boneTransform += transforms[index + boneIDs[1]] * weights[1];
	boneTransform += transforms[index + boneIDs[2]] * weights[2];
	boneTransform += transforms[index + boneIDs[3]] * weights[3];
	
	return boneTransform;
}
#endif
