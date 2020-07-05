#define BLOCK_SIZE 16
#define TILE_SIZE 16

#define POINT_LIGHT 0
#define SPOT_LIGHT 1

struct Light
{
	//uint type;
	vec4 posAndIntensityWS;
	vec4 posAndIntensityVS;
	vec4 colorAndRadius;
	//float radius;
};

struct Sphere
{
	vec3 center;
	float radius;
};

struct Plane
{
	vec3 normal;
	float d;				// Distance to origin
};

struct Frustum
{
	Plane planes[4];		// left, right, top and bottom planes
};

layout(std140, set = 0, binding = LIGHT_LIST_SSBO) readonly buffer LightListSSBO
{
	Light lights[];
};

layout(std140, set = 0, binding = OPAQUE_LIGHT_INDEX_COUNTER_SSBO) writeonly buffer OpaqueLightIndexCounter
{
	uint oLightIndexCounter[];
};

//layout(set = 0, binding = 16, r16f) uniform writeonly image2D debugTexture;

tex2D_u(0) depthTextureVS;
//layout(set = 1, binding = 0) uniform sampler2D depthTextureVS;