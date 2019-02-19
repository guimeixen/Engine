#version 450 core

layout(location = 0) in vec3 inPos;
layout(location = 1) in vec2 inUv;
layout(location = 2) in vec3 inNormal;

out vec2 uv;
out vec3 normal;
out vec3 worldPos;

uniform mat4 toWorldSpace;
uniform int instanceDataOffset;

layout(binding = 0) uniform sampler2D tex;

#include include/ubos.glsl

void main()
{
	uv = inUv;
	uv.y = 1.0 - uv.y;
	vec4 wPos;

	vec4 pos = vec4(inPos, 1.0);
	vec4 N = vec4(inNormal, 0.0);
	
	if (instanceDataOffset == -1)
	{
		normal = (toWorldSpace * N).xyz;		// Incorrect if non-uniform scale is used
		wPos = toWorldSpace * pos;	
	}
	else
	{
		mat4 t = transforms[instanceDataOffset + gl_InstanceID];
		normal = (t * N).xyz;		// Incorrect if non-uniform scale is used
		wPos = t * pos;
	}

	vec2 newuv = vec2(uv.x + timeElapsed * 0.01, uv.y);
	vec3 res = texture(tex, newuv).rgb;
	float disp = res.r;
	disp = disp * 2.0 - 1.0;
	float fineDisp = res.b;
	fineDisp = fineDisp * 2.0 - 1.0;
	wPos.x += disp * 32.0;
	//wPos.x += fineDisp * 8.0;
	
	worldPos = wPos.xyz;

	gl_Position = projView * wPos;
	
	gl_ClipDistance[0] = dot(wPos, clipPlane);
}