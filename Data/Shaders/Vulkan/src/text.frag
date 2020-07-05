#version 450
#extension GL_GOOGLE_include_directive : enable
#include "../../common.glsl"

layout(location = 0) out vec4 outColor;

layout(location = 0) in vec2 uv;
layout(location = 1) in vec4 color;

tex2D_u(0) tex;

/*layout(std140, binding = 1) uniform MaterialUBO
{
	vec4 color;
	float width;
	float edgeWidth;
	int renderText;
};*/

// Large text
//const float width = 0.51;
//const float edgeWidth = 0.02;

// Medium text
//const float width = 0.485;
//const float edgeWidth = 0.10;

// Small text
const float width = 0.46;
const float edgeWidth = 0.19;

void main()
{
	float dist = 1.0 - texture(tex, uv).a;	// The further a pixel is the higher the distance from the center. The alpha is the opposite it decreases the further from the center
															// For the correct distance we need to invert the alpha
															
	float alpha = 1.0 - smoothstep(width, width + edgeWidth, dist);	
	outColor = vec4(color.rgb, alpha * color.a);
}