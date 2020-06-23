#version 450

layout(location = 0) out vec4 outColor;

layout (location = 0) in vec2 uv;

layout(set = 1, binding = 0) uniform sampler2D image;

layout(push_constant) uniform PushConsts
{
	uint startIndex;
	uint numVecs;
	vec4 color;
	float depth;
};

void main()
{
	outColor = texture(image, uv) * color;
}