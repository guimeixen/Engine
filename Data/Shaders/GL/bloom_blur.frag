#version 450

layout(location = 0) out vec4 outColor;

in vec2 uv;

#include include/ubos.glsl

layout(binding = FIRST_SLOT) uniform sampler2D brightTexture;
//uniform bool horizontal;

layout(std140, binding = OBJECT_UBO_BINDING) uniform ObjectUBO
{
	int horizontal;
};

const float weight[5] = float[] (0.2270270270, 0.1245945946, 0.0816216216, 0.0540540541, 0.0162162162);

void main()
{
	vec2 tex_offset = 1.0 / textureSize(brightTexture, 0); // gets size of single texel
    vec3 result = texture(brightTexture, uv).rgb * weight[0];

    if (horizontal > 0)
    {
        for (int i = 1; i < 5; ++i)
        {
           result += texture(brightTexture, uv + vec2(tex_offset.x * i, 0.0)).rgb * weight[i];
           result += texture(brightTexture, uv - vec2(tex_offset.x * i, 0.0)).rgb * weight[i];         
        }
    }
    else
    {
        for (int i = 1; i < 5; ++i)
        {
			result += texture(brightTexture, uv + vec2(0.0, tex_offset.y * i)).rgb * weight[i];
            result += texture(brightTexture, uv - vec2(0.0, tex_offset.y * i)).rgb * weight[i];			
        }
    }

    outColor = vec4(result, 1.0);
}