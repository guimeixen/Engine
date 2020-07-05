// UBOs and SSBOs
#define CAMERA_UBO								0
#define INSTANCE_DATA_SSBO 						1
#define FRAME_UBO								2
#define DIR_LIGHT_UBO							3
#define FORWARD_POINT_LIGHTS_UBO				5

// Textures
#define CSM_TEXTURE								0
#define VOXEL_IMAGE								1
#define VOXEL_TEXTURE							2
#define VOXEL_IMAGE_MIPS						3




// Used only on OpenGL, but is outside the #ifdef OPENGL because otherwise we can't access it from c++ code
#define MAT_PROPERTIES_UBO_BINDING				6
#define FIRST_TEXTURE							4




#ifdef OPENGL_API



#endif


// set 0	-> UBOs
// set 1	-> SSBOs
// set 2	-> Global Textures/Images (shadow map, voxel texture, etc)
// 

// set 0	-> UBOs and SSBOs
// set 1	-> Global Textures/Images
// set 2	-> User Textures
