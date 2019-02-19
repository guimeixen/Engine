#version 450

layout(location = 0) in vec3 inPos;
layout(location = 1) in vec2 inUv;

layout(location = 0) out vec2 uv;
layout(location = 1) out vec4 clipSpacePos;
layout(location = 2) out vec3 worldPos;

//#include include/ubos.glsl				// Include using our own parser
#define VIEW_UNIFORMS_BINDING		0

layout(std140, binding = VIEW_UNIFORMS_BINDING, set = 0) uniform ViewUniforms
{
	mat4 projectionMatrix;
	mat4 viewMatrix;
	mat4 projView;
	mat4 invView;
	mat4 invProj;
	vec3 camPos;
};

//uniform mat4 toWorldSpace;

void main()
{
	uv = inUv * 16.0;
	
	//vec4 wPos = toWorldSpace * vec4(inPos, 1.0);
	vec4 wPos = vec4(inPos, 1.0);
	worldPos = wPos.xyz;
	
	clipSpacePos = projView * wPos;
	gl_Position = clipSpacePos;
}