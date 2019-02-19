#version 450

layout(location = 0) out vec4 outColor;

layout(location = 0) in float heightGradient;
layout(location = 1) in float branchIntensity;

//uniform float lightningAlpha;

layout(push_constant) uniform MatData
{
	float lightningAlpha;
};

void main()
{
	//outColor.rgb = );
	outColor.rgb =vec3(4.0 * branchIntensity) * vec3(0.8, 0.7, 1.0);
	outColor.a = heightGradient;
	outColor.a *= outColor.a * outColor.a;
	outColor.a += lightningAlpha;
	outColor.a *= branchIntensity;
	outColor.a = clamp(outColor.a, 0.0, 1.0);

	//outColor.a=1.0;
}