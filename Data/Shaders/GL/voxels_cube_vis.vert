#version 450

layout (location = 0) in vec3 inPos;

layout(location = 0) out vec3 color;

#include include/ubos.glsl

layout(binding = VOXEL_TEXTURE_SLOT) uniform sampler3D voxelTexture;
//layout(binding = 0, rgba8) uniform readonly image3D voxelTexture;

struct Voxel
{
	vec3 pos;
	float pad;
};

layout(binding = 7, std140) readonly buffer VoxelSSBO
{
	Voxel voxels[];
};

layout(std140, binding = MAT_UBO_BINDING) uniform MatUBO
{
	float volumeSize;
	float mipLevel;
};

void main()
{
	vec3 gridPos = voxels[gl_InstanceID].pos;

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