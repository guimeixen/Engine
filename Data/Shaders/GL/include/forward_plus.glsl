#define BLOCK_SIZE 16
#define TILE_SIZE 16
#define FRUSTUMS_BINDING 8
#define O_LIGHT_INDEX_LIST 10
#define LIGHT_GRID_BINDING 1

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

layout(std140, binding = 9) readonly buffer LightsSSBO
{
	Light lights[];
};

layout(std430, binding = 11) writeonly buffer OpaqueLightIndexCounter
{
	uint oLightIndexCounter[];
};

//layout(set = 0, binding = 16, r16f) uniform writeonly image2D debugTexture;

layout(binding = FIRST_SLOT) uniform sampler2D depthTextureVS;