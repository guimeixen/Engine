const vec2 gridDim = vec2(16.0, 16.0);
const vec2 size = vec2(2.0, 0.0);
const ivec3 off = ivec3(-1, 0, 1);

// gridPos - position in the terrain in the range [0,1], we can use the uv
// worldPos - world space position of the vertex to morph
// morphK - morph value
vec2 morphVertex(vec2 gridPos, vec2 worldPos, float morphK)
{
	vec2 fracPart = fract(gridPos * gridDim * 0.5) * 2.0 / gridDim;

	return worldPos - fracPart * inTrans.z * morphK;
}

vec2 calculateMorph(vec2 uv, vec3 worldPos, vec3 camPos)
{
	float dist = distance(camPos, worldPos);
	float start = inTrans.w * 0.7;	// We start morphing after 70%
	float morphConstZ = inTrans.w / (inTrans.w - start);
	float morphConstW = 1.0 / (inTrans.w - start);
	float morphK = 1.0 - clamp(morphConstZ - dist * morphConstW, 0.0, 1.0);

	return morphVertex(uv, worldPos.xz, morphK);
}

/*vec3 calculateTerrainNormal(sampler2D heightmap, vec2 uv)
{
	float left = textureOffset(heightmap, uv, off.xy).x * 256.0;
    float right = textureOffset(heightmap, uv, off.zy).x * 256.0;
    float down = textureOffset(heightmap, uv, off.yx).x * 256.0;
    float up = textureOffset(heightmap, uv, off.yz).x * 256.0;
	
	return normalize(vec3(left - right, 2.0, down - up));
}*/