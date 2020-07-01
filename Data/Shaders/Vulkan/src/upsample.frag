#version 450
#extension GL_GOOGLE_include_directive : enable
#include "include/common.glsl"

layout(location = 0) out vec4 outColor;

layout(location = 0) in vec2 uv;

tex_bind2D_user(0) brightTexture;
tex_bind2D_user(1) baseTexture;
/*layout(set = 1, binding = 0) uniform sampler2D brightTexture;
layout(set = 1, binding = 1) uniform sampler2D baseTexture;*/

void main()
{
	vec2 texelSize = 1.0 / textureSize(brightTexture, 0); 	// gets size of single texel
    
	//vec4 offset = texelSize.xyxy * vec4(-1.0, -1.0, 1.0, 1.0) * (sampleScale * 0.5);
	vec4 offset = texelSize.xyxy * vec4(-1.0, -1.0, 1.0, 1.0) * (1.0 * 0.5);
	vec3 result = texture(brightTexture, uv + offset.xy).rgb;
	result += texture(brightTexture, uv + offset.zy).rgb;
	result += texture(brightTexture, uv + offset.xw).rgb;
	result += texture(brightTexture, uv + offset.zw).rgb;
	
	result *= 0.25;
	
	vec3 base =  texture(baseTexture, uv).rgb;
	
    outColor = vec4(base + result, 1.0);
}