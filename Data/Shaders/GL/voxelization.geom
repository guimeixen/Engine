#version 450

layout(triangles) in;
layout(triangle_strip, max_vertices = 3) out;

layout(location = 0) in vec2 uvGeom[];
layout(location = 1) in vec3 normalGeom[];
layout(location = 2) in vec3 worldPosGeom[];
layout(location = 3) in vec4 lightSpacePosGeom[];

layout(location = 0) out vec2 uv;
layout(location = 1) out vec3 normal;
layout(location = 2) out vec3 worldPos; 
layout(location = 3) out vec4 lightSpacePos;
layout(location = 4) flat out int axis;

#include include/ubos.glsl

void main()
{
	// Calculate normal
	const vec3 faceN = cross(worldPosGeom[1] - worldPosGeom[0], worldPosGeom[2] - worldPosGeom[0]);
	const float NdotX = abs(faceN.x);			// NdotX = N.x * X.x + N.y * X.y + N.z * X.z = N.x * 1.0 + N.y * 0.0 + N.z *0.0 = N.x
	const float NdotY = abs(faceN.y);
	const float NdotZ = abs(faceN.z);

	mat4 proj;
		
	if (NdotZ > NdotX && NdotZ > NdotY)
	{
		axis = 3;
		proj = orthoProjZ;	
	}
	else if (NdotX > NdotY && NdotX > NdotZ)
	{
		axis = 1;
		proj = orthoProjX;
	}
	else
	{
		axis = 2;
		proj = orthoProjY;
	}
	
	gl_Position = proj * gl_in[0].gl_Position;
	normal = normalGeom[0];
	uv = uvGeom[0];
	lightSpacePos = lightSpacePosGeom[0];
	worldPos = worldPosGeom[0];
	
	EmitVertex();
	
	gl_Position = proj * gl_in[1].gl_Position;
	normal = normalGeom[1];
	uv = uvGeom[1];
	lightSpacePos = lightSpacePosGeom[1];
	worldPos = worldPosGeom[1];
	
	EmitVertex();
	
	gl_Position = proj * gl_in[2].gl_Position;
	normal = normalGeom[2];
	uv = uvGeom[2];
	lightSpacePos = lightSpacePosGeom[2];
	worldPos = worldPosGeom[2];
	
	EmitVertex();
	
	EndPrimitive();
}
