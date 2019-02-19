#version 450
#extension GL_GOOGLE_include_directive : enable

layout(location = 0) out vec4 outColor;

layout(location = 0) in vec3 worldPos;
layout(location = 1) in vec2 uv;

layout(set = 1, binding = 0) uniform sampler3D noiseTex;

#include "include/ubos.glsl"

#define STEPS 64
#define STEP_SIZE (1.0 / STEPS)
#define LIGHT_SAMPLES 16

vec3 getUVW(vec3 p)
{
	return (p + vec3(0.5));
}

bool inBounds(vec3 uvw)
{
	if (uvw.x > 1.01 ||  uvw.y > 1.01 || uvw.z > 1.01 || uvw.x < -0.01 || uvw.y < -0.01 || uvw.z < -0.01)
            return false;
			
	return true;
}

float SampleNoise(vec3 uvw)
{
	vec2 noise = texture(noiseTex, uvw).rg;
	
	float n = noise.r;

	if(n < 0.65)
		n=0.0;
		
		
	//n *= (1.0 - exp(-50.0 * uvw.y) )* exp(-4 * uvw.y * uvw.y);
	
	return n;
}

vec4 raymarch(vec3 pos, vec3 dir)
{
	vec4 acc = vec4(1.0, 1.0, 1.0, 0.0);
	
	vec3 Step = dir * STEP_SIZE;
	
	for (int i = 0; i < STEPS; i++)
	{
		vec3 uvw = getUVW(pos);
		
		float n = SampleNoise(uvw);
		
		acc.a += n * STEP_SIZE;
		
		if(acc.a > 1.0)
			break;
		
		pos += Step;
	}
	return acc;
}

void main()
{
	vec3 V = normalize(worldPos - camPos.xyz);
	outColor = raymarch(worldPos, V);
}