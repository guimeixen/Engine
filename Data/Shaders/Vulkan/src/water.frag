#version 450

layout(location = 0) out vec4 outColor;

layout(location = 0) in vec2 uv;
layout(location = 1) in vec4 clipSpacePos;
layout(location = 2) in vec3 worldPos;

layout(binding = 5, set = 0) uniform sampler2D reflectionTex;
layout(binding = 6, set = 0) uniform sampler2D normalMap;
layout(binding = 7, set = 0) uniform sampler2D dudvMap;

//#include include/ubos.glsl				// Include using our own parser
#define VIEW_UNIFORMS_BINDING		0
#define DIRLIGHT_UNIFORMS_BINDING	2
#define OBJECT_UBO_BINDING				3

layout(std140, binding = VIEW_UNIFORMS_BINDING, set = 0) uniform ViewUniforms
{
	mat4 projectionMatrix;
	mat4 viewMatrix;
	mat4 projView;
	mat4 invView;
	mat4 invProj;
	vec3 camPos;
};

layout(std140, binding = DIRLIGHT_UNIFORMS_BINDING, set = 0) uniform DirLight
{
	mat4 lightSpaceMatrix[3];
	vec4 dirAndIntensity;			// xyz - direction, z - intensity
	vec3 dirLightColor;
	vec4 cascadeEnd;
	float ambient;
};

layout(push_constant) uniform PushConstants
{
	vec2 params;			// x - moveFactor, y - waveStrength
};

void main()
{
	vec3 V = normalize(camPos - worldPos);
	vec3 H = normalize(V + dirAndIntensity.xyz);
	
	vec2 ndc = clipSpacePos.xy / clipSpacePos.w * 0.5 + 0.5;
	vec2 reflectionUV = vec2(ndc.x, 1.0 - ndc.y);
	//vec2 refractTexCoords = ndc;
	
	vec2 distortedTexCoords = texture(dudvMap, vec2(uv.x + params.x, uv.y)).rg * 0.1;	
	distortedTexCoords = uv + vec2(distortedTexCoords.x, distortedTexCoords.y + params.x);
	vec2 totalDistortion = (texture(dudvMap, distortedTexCoords).rg * 2.0 - 1.0) * params.y;
	
	reflectionUV += totalDistortion;
	reflectionUV = clamp(reflectionUV, 0.001, 0.999);
	vec3 reflection = texture(reflectionTex, reflectionUV).rgb;
	
	vec3 normal = texture(normalMap, distortedTexCoords).rgb;
	//vec3 normal = texture(normalMap, uv).rgb;
	vec3 N = vec3(normal.r * 2.0 - 1.0, normal.b * 2.0 - 1.0, normal.g * 2.0 - 1.0);		// Normal.b correponds to the up axis in tangent space
	N = normalize(N);
	
	// Fresnel
	float NdotV = dot(V, N);
	float fresnel = pow(1.0 - NdotV, 5.0);
	// Clamp otherwise some black pixelated artifacts will show
	float reflectionAmount = 8.0;
	fresnel = clamp(fresnel * reflectionAmount, 0.0, 1.0);
	
	vec3 specular = pow(max(dot(N, H), 0.0), 256.0) * dirLightColor * 4.0;
	
	outColor.rgb = mix(vec3(0.01), reflection, fresnel);
	//outColor.rgb *= 0.2;
	outColor.rgb += specular;
	
	//outColor.rgb = vec3(1.0);
	//outColor.rgb =N;
	outColor.a = 1.0;
}