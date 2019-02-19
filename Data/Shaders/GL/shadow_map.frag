#version 450
#include include/common.glsl

in vec2 uv;

layout(binding = FIRST_SLOT) uniform sampler2D tex;

void main()
{
	// Needed for alpha tested materials (grass, leaves)
	// Maybe usea separate shader for those materials. They will need
	// wind animation so I think it makes sense
	if(texture(tex, uv).a < 0.5)	
		discard;
}