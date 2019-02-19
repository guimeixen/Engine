#version 450

layout(location = 0) in vec4 posUv;
layout(location = 1) in vec4 inst_posBlendFactor;		// xyz - particle position in world space, w - particle texture blend factor
layout(location = 2) in vec4 inst_color;
layout(location = 3) in vec4 inst_texOffsets;				// Texture coords offsets used for the transition between two textures in the atlas
//layout(location = 4) in vec4 inst_rotation;				//  xyw not used    z - rotation z axis camera space

out vec2 texCoord;
out vec2 texCoord2;
out vec4 particleColor;
out float blendFactor;					// Use to blend two textures in the atlas

#include include/ubos.glsl

layout(std140, binding = OBJECT_UBO_BINDING) uniform ObjectUBO
{
	vec4 params;			// x - n of columns, y - n of rows, z - scale, w - useAtlas
};

void main()
{
	particleColor = inst_color;
	blendFactor = inst_posBlendFactor.w;

	mat4 modelMatrix = mat4(1.0);										// Constructor set all values in the diagonal to 1 (identity)
	modelMatrix[3].xyzw = vec4(inst_posBlendFactor.xyz, 1.0);			// Set the first 3 values of the third column equal to the translation and w to 1

	mat4 modelView = viewMatrix * modelMatrix;

	// Remove rotation and set the scale
	// First colunm
	/*float c = cos(inst_rotation.z);
	float s = sin(inst_rotation.z);
	
	modelView[0][0] = c * scale; 
	modelView[0][1] = s * scale;
	modelView[0][2] = 0.0;

	// Second colunm
	modelView[1][0] = -s * scale;
	modelView[1][1] = c * scale;
	modelView[1][2] = 0.0;

	// Third colunm
	modelView[2][0] = 0.0;
	modelView[2][1] = 0.0;
	modelView[2][2] = scale;*/
	
	modelView[0][0] = params.z; 
	modelView[0][1] = 0.0;
	modelView[0][2] = 0.0;

	// Second colunm
	modelView[1][0] = 0.0;
	modelView[1][1] = params.z;
	modelView[1][2] = 0.0;

	// Third colunm
	modelView[2][0] = 0.0;
	modelView[2][1] = 0.0;
	modelView[2][2] = params.z;
	
	texCoord = posUv.zw;
	texCoord.y = 1.0 - texCoord.y;
	if (params.w == 1) 
	{
		texCoord.x /= params.x;
		texCoord.y /= params.y;		
		texCoord2 = texCoord + inst_texOffsets.zw;
		texCoord += inst_texOffsets.xy;
	}

	gl_Position = projectionMatrix * modelView * vec4(posUv.xy, 0.0, 1.0);
}