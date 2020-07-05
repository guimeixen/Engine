tex3D_g(VOXEL_TEXTURE) voxelTexture;

//layout(binding = VOXEL_TEXTURE) uniform sampler3D voxelTexture;

const float volumeSize = 128;
const float samplingFactor = 1.0;
const float aoFalloff = 600.0;
//const bool enableIndirect = true;
//const float specAperture = 0.4;
const float diffAperture = 0.67;
//const float voxelGridSize = 32.0;
//const float voxelScale = 1.0 / voxelGridSize;
const float voxelSize = 1.0 / 128.0;
//const float worldSpaceVoxelSize = voxelSize * voxelGridSize;

const vec3 diffuseConeDirections[] =
{
    vec3(0.0, 1.0, 0.0),
    vec3(0.0, 0.5, 0.86602),
    vec3(0.823639, 0.5, 0.267617),
    vec3(0.509037, 0.5, -0.7006629),
    vec3(-0.50937, 0.5, -0.7006629),
    vec3(-0.823639, 0.5, 0.267617)
};

const float diffuseConeWeights[] =
{
    3.14159 / 4.0,
    3.0 * 3.14159 / 20.0,
    3.0 * 3.14159 / 20.0,
    3.0 * 3.14159 / 20.0,
    3.0 * 3.14159 / 20.0,
    3.0 * 3.14159 / 20.0,
};

vec3 orthogonal(vec3 u)
{
	//u = normalize(u);
	vec3 v = vec3(0.99146, 0.11664, 0.05832);
	return abs(dot(u, v)) > 0.999 ? cross (u, vec3(0.0, 1.0, 0.0)) : cross(u, v);
}

vec3 roundPosToGrid(vec3 pos)
{
	float interval = voxelGridSize * 0.125;			// / 8
	return round(pos / interval) * interval;
}

vec3 worldToVoxel(vec3 position)
{
	vec3 voxelGridMinPoint = vec3(-voxelGridSize * 0.5) + roundPosToGrid(camPos.xyz);		// Use half grid size here
	vec3 voxelPos = position - voxelGridMinPoint;
	return voxelPos * voxelScale;
}

vec4 traceCone(vec3 position, vec3 N, vec3 dir, float aperture)
{
	// Move a bit to avoid self collision
	float voxelWorldSize = 0.5;//0.3;
	float dist = voxelWorldSize;
	vec3 startPos = position + N * dist;
	
	vec4 coneSample = vec4(0.0);
	float occlusion = 0.0;
	float maxDistance = 2.0;
	//float falloff = 0.5 * aoFalloff * voxelScale;
	
	while(coneSample.a < 1.0 && dist <= maxDistance)
	{
		vec3 conePosition = startPos + dir * dist;
		
		// Cone expansion and respective mipmap level based on diameter
		float diameter = 2.0 * aperture * dist;
		float mipLevel = log2(diameter / voxelWorldSize);
		
		// convert position to texture coord
		vec3 uvw = worldToVoxel(conePosition);
		
		//uvw = round(uvw / voxelSize) * voxelSize;
		vec4 value = textureLod(voxelTexture, uvw, mipLevel);
		

		// front to back composition
		coneSample += (1.0 - coneSample.a) * value;

		occlusion += ((1.0 - occlusion) * value.a) / (1.0 + (dist * voxelScale) * aoIntensity * 80.0);

		// move further into the volume
		dist += diameter * samplingFactor;
	
	}
	
	return vec4(coneSample.rgb, occlusion);
}

vec4 indirectDiffuseLight(vec3 N)
{
	// Find a base for the side cones with the normal as one of it's base vectors
	const vec3 tangent = normalize(orthogonal(N));
	const vec3 bitangent = cross(tangent, N);
	
	vec4 indDiffuse = vec4(0.0);
	vec3 coneDir;

	for(int i = 0; i < 6; i++)
    {
         coneDir = N;
         coneDir += diffuseConeDirections[i].x * tangent + diffuseConeDirections[i].z * bitangent;
         coneDir = normalize(coneDir);
         indDiffuse += traceCone(worldPos, N, coneDir, diffAperture) * diffuseConeWeights[i];
    }
	
	indDiffuse.a = clamp(1.0 - indDiffuse.a + 0.01, 0.0, 1.0);
	indDiffuse.a *= indDiffuse.a * indDiffuse.a;
	indDiffuse.rgb *= giIntensity;
	return indDiffuse;
}
