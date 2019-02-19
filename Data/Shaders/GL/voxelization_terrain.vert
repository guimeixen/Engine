#version 450

layout(location = 0) in vec4 inPosUv;
layout(location = 1) in vec4 inTrans;	// xy - translation, z - size, w - range

layout(location = 0) out vec2 uvGeom;
layout(location = 1) out vec3 normalGeom;
layout(location = 2) out vec3 worldPosGeom;
layout(location = 3) out vec4 lightSpacePosGeom;
layout(location = 4) flat out vec3 colorGeom; 
layout(location = 5) out float roughnessGeom;

layout(binding = 0) uniform sampler2D heightmap;
layout(binding = 1) uniform sampler2D colorMap;
layout(binding = 2) uniform sampler2D roughnessMap;

const vec2 gridDim = vec2(16.0, 16.0);
const vec2 size = vec2(2.0, 0.0);
const ivec3 off = ivec3(-1, 0, 1);

#include include/ubos.glsl

layout(std140, binding = 1) uniform MaterialUBO
{
	vec2 terrainParams;					// x - resolution, y - height scale
	vec3 selectionPointRadius;			// xy - selection point, z - radius
};

// gridPos - position in the terrain in the range [0,1], we can use the uv
// worldPos - world space position of the vertex to morph
// morphK - morph value

vec2 morphVertex(vec2 gridPos, vec2 worldPos, float morphK)
{
	vec2 fracPart = fract(gridPos * gridDim * 0.5) * 2.0 / gridDim;

	return worldPos - fracPart * inTrans.z * morphK;
}

void main()
{
	uvGeom = inPosUv.zw;

	float texelSize = 1.0 / terrainParams.x;
	vec2 newUv = uvGeom * texelSize * inTrans.z + inTrans.xy * texelSize;
	
	float h = texture(heightmap, newUv).r * 256.0;
	worldPosGeom = vec3(uvGeom.x * inTrans.z + inTrans.x, h, uvGeom.y * inTrans.z + inTrans.y);

	float dist = distance(camPos.xyz, worldPosGeom);
	float start = inTrans.w * 0.7;	// We start morphing at after 70%
	float morphConstZ = inTrans.w / (inTrans.w - start);
	float morphConstW = 1.0 / (inTrans.w - start);
	float morphK = 1.0 - clamp(morphConstZ - dist * morphConstW, 0.0, 1.0);
	
	worldPosGeom.xz = morphVertex(uvGeom, worldPosGeom.xz, morphK);
	
	newUv = worldPosGeom.xz * texelSize;
	uvGeom = newUv;
	h = texture(heightmap, newUv).r * 256.0;

	// Normal calculation
	float left = textureOffset(heightmap, uvGeom, off.xy).x * 256.0;
    float right = textureOffset(heightmap, uvGeom, off.zy).x * 256.0;
    float down = textureOffset(heightmap, uvGeom, off.yx).x * 256.0;
    float up = textureOffset(heightmap, uvGeom, off.yz).x * 256.0;
	normalGeom = normalize(vec3(left - right, 2.0, down - up));
	
	worldPosGeom.y = (h + left + right + up + down) * 0.2 * terrainParams.y;
	
	vec4 mapColor = texture(colorMap, newUv);

	colorGeom = mapColor.rgb * mapColor.a;
	roughnessGeom = texture(roughnessMap, newUv).r;
	
	gl_Position = vec4(worldPosGeom, 1.0);
	
	lightSpacePosGeom = lightSpaceMatrix[0] * vec4(worldPosGeom, 1.0);
	lightSpacePosGeom.xyz = lightSpacePosGeom.xyz / lightSpacePosGeom.w;
	lightSpacePosGeom.xy = lightSpacePosGeom.xy * 0.5 + 0.5;		// Only multiply the xy because the z is already in [0,1] (set with glClipControl)
	//lightSpacePos[0] = lightSpaceMatrix[0] * wPos;		
	//lightSpacePos[1] = lightSpaceMatrix[1] * wPos;
	//lightSpacePos[2] = lightSpaceMatrix[2] * wPos;

	//clipSpaceDepth = gl_Position.w / nearFarPlane.y;
}

