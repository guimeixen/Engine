#version 450
#extension GL_GOOGLE_include_directive : enable

layout(location = 0) in vec4 inPosUv;
layout(location = 1) in vec4 inTrans;	// xy - translation, z - size, w - range

layout(location = 0) out vec2 uv;
layout(location = 1) out vec3 worldPos;
layout(location = 2) flat out vec3 normal;
layout(location = 3) out vec3 smoothNormal;
layout(location = 4) out float clipSpaceDepth;
layout(location = 5) out vec3 color;
layout(location = 6) out vec4 lightSpacePos[3];

layout(set = 1, binding = 0) uniform sampler2D heightmap;
layout(set = 1, binding = 1) uniform sampler2D colorMap;

#include "include/terrain.glsl"
#include "include/ubos.glsl"

/*struct TerrainVertex
{
	vec3 worldPos;
	vec3 normal;
};*/

void main()
{	
	uv = inPosUv.zw;
	
	/*TerrainVertex tv = calculateTerrainVertex();
	worldPos = tv.worldPos;
	normal = tv.normal;
	smoothNormal = normal;*/
	
	float terrainRes = textureSize(heightmap, 0).r;
	float texelSize = 1.0 / terrainRes;
	vec2 newUv = uv * texelSize * inTrans.z + inTrans.xy * texelSize;
	
	float h = texture(heightmap, newUv).r * 256.0;
	worldPos = vec3(uv.x * inTrans.z + inTrans.x, h, uv.y * inTrans.z + inTrans.y);
	worldPos.xz = calculateMorph(uv, worldPos, camPos.xyz);

	newUv = worldPos.xz * texelSize;
	uv = newUv;
	h = texture(heightmap, newUv).r * 256.0;

	// Normal calculation
	float left = textureOffset(heightmap, uv, off.xy).x * 256.0;
    float right = textureOffset(heightmap, uv, off.zy).x * 256.0;
    float down = textureOffset(heightmap, uv, off.yx).x * 256.0;
    float up = textureOffset(heightmap, uv, off.yz).x * 256.0;
	normal = normalize(vec3(left - right, 2.0, down - up));
	smoothNormal = normal;
	
	color = texture(colorMap, newUv).rgb;
	
	worldPos.y = (h + left + right + up + down) * 0.2;

	vec4 wPos = vec4(worldPos, 1.0);
	gl_Position = projView * wPos;
	
	lightSpacePos[0] = lightSpaceMatrix[0] * wPos;		
	lightSpacePos[1] = lightSpaceMatrix[1] * wPos;
	lightSpacePos[2] = lightSpaceMatrix[2] * wPos;
	
	gl_ClipDistance[0] = dot(wPos, clipPlane);

	clipSpaceDepth = gl_Position.w / nearFarPlane.y;
}
