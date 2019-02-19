#version 450
#include include/common.glsl

layout(location = 0) out vec4 outColor;

in vec2 uv;
in vec3 normal;
in vec3 worldPos;

layout(binding = FIRST_SLOT) uniform sampler2D tex;

void main()
{
	vec4 diffuseMap = texture(tex, uv);
	
	if (diffuseMap.a < 0.35)
		discard;
	
	outColor.rgb = diffuseMap.rgb;
	outColor.a = 1.0;
}