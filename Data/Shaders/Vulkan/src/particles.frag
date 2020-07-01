#version 450
#extension GL_GOOGLE_include_directive : enable
#include "include/ubos.glsl"

layout(location = 0) out vec4 color;

layout(location = 0) in vec2 texCoord;
layout(location = 1) in vec2 texCoord2;
layout(location = 2) in vec4 particleColor;
layout(location = 3) in float blendFactor;

tex_bind2D_user(0) particleTexture;
//layout(set = 1, binding = 0) uniform sampler2D particleTexture;

layout(push_constant) uniform PushConsts
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

	color.rgb *= color.a;								
}