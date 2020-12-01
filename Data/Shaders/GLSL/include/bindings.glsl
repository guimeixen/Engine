// UBOs and SSBOs
#define CAMERA_UBO								0
#define INSTANCE_DATA_SSBO 						1
#define FRAME_UBO								2
#define DIR_LIGHT_UBO							3
#define FORWARD_POINT_LIGHTS_UBO				5		// These two can use the same binding because the ubo is just used during forward rendering
#define FRUSTUMS_SSBO							5		// and the ssbo during forward plus
#define LIGHT_LIST_SSBO							6
#define OPAQUE_LIGHT_INDEX_LIST_SSBO			7
#define OPAQUE_LIGHT_INDEX_COUNTER_SSBO			8
#define DEBUG_VOXELS_INDIRECT_DRAW				9
#define DEBUG_VOXELS_POSITION_SSBO				10
#define BUFFERS_COUNT							(DEBUG_VOXELS_POSITION_SSBO + 1)

// Textures
#define CSM_TEXTURE								0
#define VOXEL_IMAGE								1
#define VOXEL_TEXTURE							2
#define VOXEL_IMAGE_MIPS						3
#define LIGHT_GRID_TEXTURE						4
#define TEXTURE_COUNT							(LIGHT_GRID_TEXTURE + 1)



// Used only on OpenGL, but is outside the #ifdef OPENGL because otherwise we can't access it from c++ code
#define MAT_PROPERTIES_UBO_BINDING				BUFFERS_COUNT
#define FIRST_TEXTURE							TEXTURE_COUNT




#ifdef OPENGL_API



#endif


// set 0	-> UBOs
// set 1	-> SSBOs
// set 2	-> Global Textures/Images (shadow map, voxel texture, etc)
// 

// set 0	-> UBOs and SSBOs
// set 1	-> Global Textures/Images
// set 2	-> User Textures
