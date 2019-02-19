#version 450

layout(location = 0) in vec3 inPos;
layout(location = 1) in vec2 inUv;
layout(location = 2) in vec3 inNormal;
#ifdef INSTANCING
layout(location = 3) in mat4 instanceMatrix;
#endif

out vec2 uv;
out vec3 normal;
out vec3 worldPos;
out vec4 lightSpacePos[3];
out float clipSpaceDepth;

#include include/ubos.glsl

// worldPos : 		world position of the plant relative to the base of the plant
//						we assume the base is at (0,0,0). Ensure this before calling this function
/*vec3 MainBending(vec3 worldPos, vec3 objPos)
{
	vec3 vPos = worldPos - objPos;
	
	// Calculate the length fromt the ground
	float lengthFromGround = length(vPos);
	// Bend factor - wind variation done on the cpu
	float bf = vPos.y * bendScale;
	// Smooth bending factor and increase its nearby height limit
	bf += 1.0;
	bf *= bf;
	bf = bf * bf - bf;
	// Displace position
	vPos.xz += windDirStr.xy * bf;
	// Rescale - this keeps the plants parts form stretching by shortening the y (height) while
	// they move about the xz
	vPos = normalize(vPos) * lengthFromGround;
	vPos += objPos;
	
	return vPos;
}*/

void main()
{
	uv = inUv;
		
	normal = (instanceMatrix * vec4(inNormal, 0.0)).xyz;
	vec4 wPos = instanceMatrix * vec4(inPos, 1.0);

	worldPos = wPos.xyz;
	
	gl_Position = projView * wPos;

	// Vertex position in light's clip space	range [-1,1] instead of [-w,w]. No need to divide by w because it's an orthographic projection and so w is unused
	lightSpacePos[0] = lightSpaceMatrix[0] * wPos;		
	lightSpacePos[1] = lightSpaceMatrix[1] * wPos;
	lightSpacePos[2] = lightSpaceMatrix[2] * wPos;

	clipSpaceDepth = gl_Position.w / nearFarPlane.y;
}