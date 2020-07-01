#define VOXEL_TEXTURE_SLOT 0
#define SHADOW_MAP_SLOT 1
#define FIRST_SLOT 2

#define CASCADE_COUNT 3
#define ONE_OVER_CASCADE_COUNT (1.0 / CASCADE_COUNT)
#define SHADOW_MAP_RES 2048
#define ONE_OVER_SHADOW_MAP_RES (1.0 / SHADOW_MAP_RES)		// Shadow map pixel size

// Lighting
#define MAX_POINT_LIGHTS 8

#define texture_bind3D(b) layout(binding = FIRST_SLOT + b) uniform sampler3D
#define texture_bind2D(b) layout(binding = FIRST_SLOT + b) uniform sampler2D

//#define VIEW_UNIFORMS_BINDING				0
//#define MAT_UBO_BINDING							2
//#define DIRLIGHT_UNIFORMS_BINDING			2
//#define OBJECT_UBO_BINDING						3
//#define POINTLIGHT_UNIFORMS_BINDING		4
//#define INSTANCE_DATA_BINDING					5
#define VOXEL_POSITIONS_BUFFER				7
#define VOXEL_VIS_INDIRECT_BUFFER			8
//#define FRAME_UBO_BINDING						9*/

