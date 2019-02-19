struct PixelInput
{
	float4 position : SV_POSITION;
	float3 color : NORMAL;
};

float4 PS(PixelInput i) : SV_TARGET
{
	return float4(i.color, 1.0);
}