#version 450

layout(location = 0) in vec3 pos;

layout(location = 0) out vec3 uv;

//#include include/ubos.glsl				// Include using our own parser
#define VIEW_UNIFORMS_BINDING 0

layout(std140, binding = VIEW_UNIFORMS_BINDING, set = 0) uniform ViewUniforms
{
	mat4 projectionMatrix;
	mat4 viewMatrix;
	mat4 projView;
	mat4 invView;
	mat4 invProj;
	vec3 camPos;
};

void main()
{
	uv = pos;
	mat4 m = mat4(1.0);
	m[3] = vec4(camPos, 1.0);
	gl_Position = projView * m * vec4(pos, 1.0);
	gl_Position = gl_Position.xyww;
}