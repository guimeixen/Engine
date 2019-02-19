#version 450
#include include/ubos.glsl

layout(location = 0) out vec4 outColor;

in vec2 uv;

layout(binding = FIRST_SLOT) uniform sampler2D brightTexture;
layout(binding = FIRST_SLOT + 1) uniform sampler2D baseTexture;

/*layout(std140, binding = OBJECT_UBO_BINDING) uniform ObjectUBO
{
	float sampleScale;
};*/

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