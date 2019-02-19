#version 330 core
#extension GL_ARB_shading_language_420pack: enable

layout(location = 0) out vec4 color;

in vec2 uv;

layout(binding = 0) uniform sampler2D tex;

void main()
{
	color = texture(tex, uv);
	
	if (color.a < 0.5)
		discard;

	color = vec4(0.0, 0.0, 0.0, 1.0);
}