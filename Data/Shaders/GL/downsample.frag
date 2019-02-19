#version 450
#include include/common.glsl

layout(location = 0) out vec4 outColor;

in vec2 uv;

layout(binding = FIRST_SLOT) uniform sampler2D tex;

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