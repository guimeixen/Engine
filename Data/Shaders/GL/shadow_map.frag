#version 450
#include "../common.glsl"

layout(location = 0) in vec2 uv;

tex2D_u(0) tex;

void main()
{
	// Needed for alpha tested materials (grass, leaves)
	// Maybe usea separate shader for those materials. They will need
	// wind animation so I think it makes sense
	if(texture(tex, uv).a < 0.5)	
		discard;
}