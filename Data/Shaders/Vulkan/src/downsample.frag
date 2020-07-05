#version 450
#extension GL_GOOGLE_include_directive : enable
#include "../../common.glsl"

layout(location = 0) out vec4 outColor;

layout(location = 0) in vec2 uv;

tex2D_u(0) tex;

void main()
{
	vec2 texelSize = 1.0 / textureSize(tex, 0); 	// gets size of single texel
    
	vec4 offset = texelSize.xyxy * vec4(-1.0, -1.0, 1.0, 1.0);
	
	vec3 result = texture(tex, uv + offset.xy).rgb;
	result += texture(tex, uv + offset.zy).rgb;
	result += texture(tex, uv + offset.xw).rgb;
	result += texture(tex, uv + offset.zw).rgb;
	
	//result *= 1.0 / 4.0;
	result *= 0.25;
	
    outColor = vec4(result, 1.0);
}