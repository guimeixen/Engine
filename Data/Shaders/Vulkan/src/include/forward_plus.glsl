#define BLOCK_SIZE 16
#define TILE_SIZE 16
#define FRUSTUMS_BINDING 11
#define O_LIGHT_INDEX_LIST 13
#define LIGHT_GRID_BINDING 15

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

layout(std140, set = 0, binding = 12) readonly buffer LightsSSBO
{
	Light lights[];
};

layout(std140, set = 0, binding = 14) writeonly buffer OpaqueLightIndexCounter
{
	uint oLightIndexCounter[];
};

//layout(set = 0, binding = 16, r16f) uniform writeonly image2D debugTexture;

layout(set = 1, binding = 0) uniform sampler2D depthTextureVS;