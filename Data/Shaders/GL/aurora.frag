#version 450 core

layout(location = 0) out vec4 outColor;
layout(location = 1) out vec4 outNormal;

in vec2 uv;
in vec3 normal;
in vec3 worldPos;

layout(binding = 0) uniform sampler2D tex;

#include include/ubos.glsl

void main()
{
	//vec3 N = normalize(normal);
	outNormal.rgb = vec3(1.0);
	outNormal.a = 1.0;

	vec2 newuv = vec2(uv.x * 4.0 + timeElapsed * 0.01, uv.y);
	
	float alpha = texture(tex, vec2(uv.x , uv.y)).g;
	float detail = texture(tex, newuv).b;
		
	vec3 topColor = vec3(0.65, 0.02, 0.27);
	vec3 bottomColor = vec3(0.0, 1.0, 0.31);
	
	float m = smoothstep(0.15, 1.0, uv.y * uv.y);
	vec3 auroraColor = mix(topColor, bottomColor * 4.0, m );

	float intensity = sin(-newuv.x*8.0 + timeElapsed) * 0.5 + 0.5;
	intensity *= 2.5;
	intensity = clamp(intensity, 0.4, 8.0);
	
	outColor.rgb = auroraColor  * detail * 0.2 * intensity;
	
	outColor.a = cos(timeOfDay * 0.23) * 2 - 0.2;
	outColor.a = clamp(outColor.a * alpha, 0.0, 1.0);
}