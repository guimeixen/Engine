#version 450
#extension GL_GOOGLE_include_directive : enable
#include "include/common.glsl"

layout(location = 0) in vec2 uv;

//layout(set = 2, binding = 0) uniform sampler2D tex;
tex2D_u(0) tex;

void main()
{
	// Needed for alpha tested materials (grass, leaves)
	// Maybe usea separate shader for those materials. They will also need wind animation
	if(texture(tex, uv).a < 0.5)	
		discard;
}