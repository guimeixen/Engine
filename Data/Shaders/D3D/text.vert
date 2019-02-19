cbuffer CameraCB : register( b0 )
{
	float4x4 proj;
	float4x4 view;
	float4x4 projView;
	float4x4 invView;
	float4x4 invProj;
	float4 clipPlane;
	float4 camPos;
	float2 nearFarPlane;
};

struct VertexInput
{
	float4 posUv : POSITION;
	float4 color : NORMAL;
};

struct PixelInput
{
	float4 position : SV_POSITION;
	float2 uv : TEXCOORD0;
	float4 color : TEXCOORD1;
};

PixelInput VS(VertexInput i)
{
	PixelInput o;

	o.position = mul(float4(i.posUv.x, i.posUv.y , 0.0, 1.0), projView);
	o.uv = float2(i.posUv.z, i.posUv.w);
	o.color = i.color;

	return o;
}