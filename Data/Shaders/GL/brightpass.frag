#version 450
#include "../common.glsl"

layout(location = 0) out vec4 outColor;

in vec2 uv;

tex2D_u(0) sceneHDR;
//layout(binding = FIRST_SLOT) uniform sampler2D sceneHDR;

/*layout(std140, binding = OBJECT_UBO_BINDING) uniform ObjectUBO
{
	float threshold;
};*/

float brightness(vec3 rgb)
{
	return max(rgb.x, max(rgb.y, rgb.z));
}

void main()
{
	vec3 sceneColor = texture(sceneHDR, uv).rgb;
	float br = brightness(sceneColor);

     // Under-threshold part: quadratic curve
     /*float rq = clamp(br - _Curve.x, 0.0, _Curve.y);
     rq = _Curve.z * rq * rq;*/

     // Combine and apply the brightness response curve.
	// float f = max(br - threshold, 0.0);
	float f = max(br - 1.6, 0.0);
     sceneColor *= f / max(br, 1e-5);
	
    outColor = vec4(sceneColor, 1.0);
}