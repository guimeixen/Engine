#version 450

layout(location = 0) in vec2 uv;

layout(set = 1, binding = 0) uniform sampler2D tex;

void main()
{
	// Needed for alpha tested materials (grass, leaves)
	// Maybe usea separate shader for those materials. They will also need wind animation
	if(texture(tex, uv).a < 0.5)	
		discard;
}