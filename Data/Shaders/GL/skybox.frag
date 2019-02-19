#version 450
#include include/common.glsl

layout(location = 0) out vec4 color;

in vec3 uv;

layout(binding = FIRST_SLOT) uniform samplerCube skybox;

void main()
{
	color = texture(skybox, uv);
}