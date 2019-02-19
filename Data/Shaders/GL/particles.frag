#version 450
#include include/ubos.glsl

layout(location = 0) out vec4 color;

in vec2 texCoord;
in vec2 texCoord2;
in vec4 particleColor;
in float blendFactor;

layout(binding = FIRST_SLOT) uniform sampler2D particleTexture;

layout(std140, binding = OBJECT_UBO_BINDING) uniform ObjectUBO
{
	vec4 params;			// x - n of columns, y - n of rows, z - scale, w - useAtlas
};

void main()
{
	if (params.w > 0) 
	{
		vec4 color1 = texture(particleTexture, texCoord);
		vec4 color2 = texture(particleTexture, texCoord2);
		vec4 mixed = mix(color1, color2, blendFactor);
		color = mixed * particleColor;
	}
	else
	{
		vec4 texDiff = texture(particleTexture, texCoord);
		color = texDiff * particleColor;
	}
	
	//color.rgb *= 2.15;		// Add as uniform emissive
	color.rgb *= color.a;								
}