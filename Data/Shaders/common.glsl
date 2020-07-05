#include "bindings.glsl"

#define CASCADE_COUNT 3
#define ONE_OVER_CASCADE_COUNT (1.0 / CASCADE_COUNT)
#define SHADOW_MAP_RES 2048
#define ONE_OVER_SHADOW_MAP_RES (1.0 / SHADOW_MAP_RES)		// Shadow map pixel size

// Lighting
#define MAX_POINT_LIGHTS 8

#ifdef OPENGL_API

// Global Textures
#define tex2D_g(b) layout(binding = b) uniform sampler2D
#define tex2DShadow_g(b) layout(binding = b) uniform sampler2DShadow
#define tex3D_g(b) layout(binding = b) uniform sampler3D

// Global Images
#define image3D_g(b, format, mode) layout(binding = b, format) uniform mode image3D
#define uimage2D_g(b, format, mode) layout(binding = b, format) uniform mode uimage2D

// User Textures
#define tex2D_u(b) layout(binding =  FIRST_TEXTURE + b) uniform sampler2D
#define tex2DShadow_u(b) layout(binding =  FIRST_TEXTURE + b) uniform sampler2DShadow
#define tex3D_u(b) layout(binding =  FIRST_TEXTURE + b) uniform sampler3D

#define PROPERTIES layout(std140, binding = MAT_PROPERTIES_UBO_BINDING) uniform MatUBO

#elif VULKAN_API

#define BUFFERS_SET 0
#define TEXTURES_SET 1
#define USER_SET 2

// Global Textures
#define tex2D_g(b) layout(set = TEXTURES_SET, binding = b) uniform sampler2D
#define tex2DShadow_g(b) layout(set = TEXTURES_SET, binding = b) uniform sampler2DShadow
#define tex3D_g(b) layout(set = TEXTURES_SET, binding = b) uniform sampler3D

// Global Images
#define image3D_g(b, format, mode) layout(set = 1, binding = b, format) uniform mode image3D
#define uimage2D_g(b, format, mode) layout(set = 1, binding = b, format) uniform mode uimage2D

// User Textures
#define tex2D_u(b) layout(set = USER_SET, binding = b) uniform sampler2D
#define tex2DShadow_u(b) layout(set = USER_SET, binding = b) uniform sampler2DShadow
#define tex3D_u(b) layout(set = USER_SET, binding = b) uniform sampler3D

#define PROPERTIES layout(push_constant) uniform PushConsts

#endif
