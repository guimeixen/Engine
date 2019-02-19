#version 450

layout(location = 0) in vec4 posUv;

layout(location = 0) out vec2 uv;
layout(location = 1) out vec4 clipSpacePos;
layout(location = 2) flat out vec3 normal;
layout(location = 3) out vec3 worldPos;
layout(location = 4) flat out vec3 specular;

uniform mat4 toWorldSpace;

#include include/ubos.glsl

/*const float waveLength[] = { 1.0, 2.0, 4.0 };
const float amplitude[] = { 0.5, 0.2, 0.05 };
const float speed[] = { 1.0, 0.5, 0.2 };
const vec2 dir[] = vec2[] (vec2(-0.1, 0.9), vec2(0.2, 0.8), vec2(-0.7, 0.3));*/

const float wavelength[4] = float[](6.0, 3.8, 1.9, 0.3);
const float amplitude[4] = float[](0.35, 0.21, 0.1, 0.05);
const float speed[4] = float[](2.4, 1.8, 1.0, 0.2);
const vec2 dir[4] = vec2[](vec2(0.71, -0.7), vec2(-0.5, 0.87), vec2(-0.98, 0.18), vec2(0.43, 0.9));

vec3 waveNormal(vec2 worldPos)
{
	vec3 n = vec3(0.0);
	for (int i = 0; i < 4; i++)
	{
		float w = 2.0 / wavelength[i];
		float s = speed[i] * w;
		
		float x = w * dir[i].x * amplitude[i] * cos(dot(dir[i], worldPos) * w + timeElapsed * s);
		float z = w * dir[i].y * amplitude[i] * cos(dot(dir[i], worldPos) * w + timeElapsed * s);
	
		n += vec3(-x, 1.0, -z);
	}

	n = normalize(n);
	
	return n;
}

float waveHeight(vec2 worldPos)
{
	float height = 0.0;
	for (int i = 0; i < 4; i++)
	{
		float w = 2.0 / wavelength[i];
		float s = speed[i] * w;
		height += amplitude[i] * sin(dot(dir[i], worldPos) * w + timeElapsed * s);
	}

	return height;
}

void main()
{
	uv = posUv.zw;
	
	vec4 worldPos4 = toWorldSpace * vec4(posUv.x * 6.0, 0.0, posUv.y * 6.0, 1.0);
	
	worldPos4.y += waveHeight(worldPos4.xz);
	
	normal = waveNormal(worldPos4.xz);
	
	worldPos = worldPos4.xyz;
	
	clipSpacePos = projView * worldPos4;
	gl_Position = clipSpacePos;
	
	vec3 V = normalize(camPos.xyz - worldPos);
	vec3 H = normalize(V + dirAndIntensity.xyz);
	
	specular = pow(max(dot(normal, H), 0.0), 256.0) * dirLightColor * 4.0;
	
}