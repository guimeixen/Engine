#version 450

layout(location = 0) in vec4 posuv;

layout (location = 0) out vec2 uv;

void main()
{	
	uv = GET_UV(posuv.zw);
	gl_Position = CLIP_FROM_2DVERT(posuv.xy);
}
