#version 450

layout(location = 0) out vec4 outColor;

layout(location = 0) in vec3 normal;

void main()
{
	vec3 L = normalize(vec3(1.0, 1.0, 1.0));
	vec3 N = normalize(normal);
	
	float diff = max(dot(N,L), 0.0);
	vec3 lighting = vec3(diff) + vec3(0.05);
	
	outColor = vec4(lighting, 1.0);
}