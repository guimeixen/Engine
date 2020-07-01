#version 450
#extension GL_GOOGLE_include_directive : enable
#include "include/common.glsl"

layout(location = 0) out vec4 color;

layout(location = 0) in vec3 uv;

tex_bind2D_global(5) skybox;
//layout(binding = 5, set = 0) uniform samplerCube skybox;

void main()
{
	color = texture(skybox, uv);
}