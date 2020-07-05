#version 450
#extension GL_GOOGLE_include_directive : enable
#include "include/ubos.glsl"

layout(location = 0) out vec4 outColor;

layout(location = 0) in vec2 uv;

tex2D_u(0) sceneHDR;

float brightness(vec3 rgb)
{
	return max(rgb.x, max(rgb.y, rgb.z));
}

void main()
{
	vec3 sceneColor = texture(sceneHDR, uv).rgb;
	float br = brightness(sceneColor);

     // Combine and apply the brightness response curve.
	 // float f = max(br - threshold, 0.0);
	 float f = max(br - bloomThreshold, 0.0);
     sceneColor *= f / max(br, 1e-5);
	
    outColor = vec4(sceneColor, 1.0);
}