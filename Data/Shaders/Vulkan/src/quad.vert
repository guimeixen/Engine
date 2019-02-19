#version 450

layout(location = 0) in vec4 posuv;

layout (location = 0) out vec2 uv;

void main()
{
	uv = vec2(posuv.z, 1.0 - posuv.w);
	gl_Position = vec4(posuv.x, posuv.y * -1.0, 0.1, 1.0);
}