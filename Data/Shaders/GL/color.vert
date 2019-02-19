#version 450

layout(location = 0) in vec4 posuv;

out vec2 uv;

uniform mat4 projMatrix;
uniform mat4 modelMatrix;

void main()
{
	uv = vec2(posuv.z, posuv.w);
	//gl_Position = projMatrix * modelMatrix * vec4(posuv.x + 1.0, posuv.y + 1.0, 0.0, 1.0);		// +1 so the quad gets placed using the bottom left instead of the center
	gl_Position = projMatrix * modelMatrix * vec4(posuv.xy, 0.0, 1.0);
}