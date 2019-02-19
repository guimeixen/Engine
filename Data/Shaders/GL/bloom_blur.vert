#version 450

layout(location = 0) in vec4 posuv;

out vec2 uv;

void main()
{
	uv = posuv.zw;
	gl_Position = vec4(posuv.xy, 0.1, 1.0);
}