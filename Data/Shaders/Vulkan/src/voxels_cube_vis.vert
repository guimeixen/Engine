#version 450
#extension GL_GOOGLE_include_directive : enable
#include "include/ubos.glsl"

layout(location = 0) in vec3 inPos;

layout(location = 0) out vec3 color;

tex3D_g(VOXEL_TEXTURE) voxelTexture;

struct Voxel
{
	vec3 pos;
	float pad;
};

layout(set = 0, binding = DEBUG_VOXELS_POSITION_SSBO, std140) readonly buffer VoxelSSBO
{
	Voxel voxels[];
};

PROPERTIES
{
	float volumeSize;
	float mipLevel;
};

void main()
{
	vec3 gridPos = voxels[gl_InstanceIndex].pos;

	float scale = voxelGridSize / volumeSize;		// Grid size = 64, volume size = 64 then scale = 1
	float halfSize = voxelGridSize * 0.5;
	
	// Snap the camera so we move in voxel sized increments
	float interval = voxelGridSize * 0.125;  // / 8
	vec3 newCamPos = round(camPos.xyz / interval) * interval;
	
	mat4 m = mat4(scale);
	m[3] = vec4(gridPos * scale - halfSize + newCamPos, 1.0);
	
	color = textureLod(voxelTexture, gridPos / volumeSize, mipLevel).rgb;

	gl_Position = projView * m * vec4(inPos, 1.0);
}