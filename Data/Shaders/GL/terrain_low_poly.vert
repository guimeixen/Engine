#version 450

layout(location = 0) in vec4 inPosUv;
layout(location = 1) in vec4 inTrans;	// xy - translation, z - size, w - range

out vec2 uv;
out float morph;
out vec3 worldPos;
flat out vec3 normal;
out vec3 smoothNormal;
flat out vec3 color;
out vec4 lightSpacePos[4];
out float clipSpaceDepth;

layout(binding = 0) uniform sampler2D heightmap;
layout(binding = 1) uniform sampler2D colorMap;

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
	uv = inPosUv.zw;
	float texelSize = 1.0 / terrainParams.x;
	vec2 newUv = uv * texelSize * inTrans.z + inTrans.xy * texelSize;
	
	float h = texture(heightmap, newUv).r * 256.0;
	worldPos = vec3(uv.x * inTrans.z + inTrans.x, h, uv.y * inTrans.z + inTrans.y);

	float dist = distance(camPos.xyz worldPos);
	float start = inTrans.w * 0.7;	// We start morphing at after 70%
	float morphConstZ = inTrans.w / (inTrans.w - start);
	float morphConstW = 1.0 / (inTrans.w - start);
	float morphK = 1.0 - clamp(morphConstZ - dist * morphConstW, 0.0, 1.0);

	morph = morphK;
	worldPos.xz = morphVertex(uv, worldPos.xz, morphK);

	newUv = worldPos.xz * texelSize;
	uv = newUv;
	h = texture(heightmap, newUv).r * 256.0;

	// Normal calculation
	float left = textureOffset(heightmap, uv, off.xy).x * 256.0;
    float right = textureOffset(heightmap, uv, off.zy).x * 256.0;
    float down = textureOffset(heightmap, uv, off.yx).x * 256.0;
    float up = textureOffset(heightmap, uv, off.yz).x * 256.0;
	normal = normalize(vec3(left - right, 2.0, down - up));
	smoothNormal=normal;
	
	color = texture(colorMap, newUv).rgb;
	
	// Avg the height from the 5 samples above
	worldPos.y = (h + left + right + up + down) * 0.2 * terrainParams.y;

	gl_Position = projectionMatrix * viewMatrix * vec4(worldPos, 1.0);

	// Vertex position in light's clip space	range [-1,1] instead of [-w,w]. No need to divide by w because it's an orthographic projection and so w is unused
	lightSpacePos[0] = lightSpaceMatrix[0] * vec4(worldPos, 1.0);		
	lightSpacePos[1] = lightSpaceMatrix[1] * vec4(worldPos, 1.0);
	lightSpacePos[2] = lightSpaceMatrix[2] * vec4(worldPos, 1.0);
	lightSpacePos[3] = lightSpaceMatrix[3] * vec4(worldPos, 1.0);

	clipSpaceDepth = gl_Position.w / nearFarPlane.y;		// far plane
}
