#version 450

layout(location = 0) in vec4 posuv;

layout (location = 0) out vec2 uv;

void main()
{
	//vk
	uv = vec2(posuv.z, 1.0 - posuv.w);
	gl_Position = vec4(posuv.x, posuv.y * -1.0, 0.0, 1.0);
	
	//gl
	uv = posuv.zw;
	gl_Position = vec4(posuv.xy, 0.1, 1.0);
	
	uv = GET_UV(posuv.zw);
}

#ifdef OPENGL

#define GET_UV(uv) uv

#elif VULKAN

#define GET_UV(uv) vec2(uv.x, 1.0 - uv.w)

#endif
