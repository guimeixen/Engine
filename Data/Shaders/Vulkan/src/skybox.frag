#version 450

layout(location = 0) out vec4 color;

layout(location = 0) in vec3 uv;

layout(binding = 5, set = 0) uniform samplerCube skybox;

void main()
{
	color = texture(skybox, uv);
}