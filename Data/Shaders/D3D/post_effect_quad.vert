struct VertexInput
{
	float4 posUv : POSITION;
};

struct PixelInput
{
	float4 position : SV_POSITION;
	float2 uv : TEXCOORD0;
};

PixelInput VS(VertexInput i)
{
	PixelInput o;

	o.position = float4(i.posUv.x, i.posUv.y, 0.1, 1.0);
	o.uv = float2(i.posUv.z, 1.0-i.posUv.w);

	return o;
}