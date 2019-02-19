#include "include/ubos.hlsli"

struct GeomInput
{
	float4 position : SV_POSITION;
	float2 uv : TEXCOORD0;
	float3 worldPos : TEXCOORD1;
	float3 normal : TEXCOORD2;
	float4 lightSpacePos : TEXCOORD3;
};

struct PixelInput
{
	float4 position : SV_POSITION;
	float2 uv : TEXCOORD0;
	float3 worldPos : TEXCOORD1;
	float3 normal : TEXCOORD2;
	float4 lightSpacePos : TEXCOORD3;
	nointerpolation int axis : TEXCOORD4;
};

[maxvertexcount(3)]
void GS(triangle GeomInput input[3], inout TriangleStream<PixelInput> triStream)
{
	// Calculate normal
	const float3 faceN = cross(input[1].worldPos - input[0].worldPos, input[2].worldPos - input[0].worldPos);
	const float NdotX = abs(faceN.x);			// NdotX = N.x * X.x + N.y * X.y + N.z * X.z = N.x * 1.0 + N.y * 0.0 + N.z *0.0 = N.x
	const float NdotY = abs(faceN.y);
	const float NdotZ = abs(faceN.z);

	PixelInput o;
	float4x4 proj;
		
	if (NdotZ > NdotX && NdotZ > NdotY)
	{
		o.axis = 3;
		proj = orthoProjZ;	
	}
	else if (NdotX > NdotY && NdotX > NdotZ)
	{
		o.axis = 1;
		proj = orthoProjX;
	}
	else
	{
		o.axis = 2;
		proj = orthoProjY;
	}

	proj = transpose(proj);
	
	o.position = mul(input[0].position, proj);
	o.normal = input[0].normal;
	o.uv = input[0].uv;
	o.lightSpacePos = input[0].lightSpacePos;
	o.worldPos = input[0].worldPos;
	
	triStream.Append(o);
	
	o.position = mul(input[1].position, proj);
	o.normal = input[1].normal;
	o.uv = input[1].uv;
	o.lightSpacePos = input[1].lightSpacePos;
	o.worldPos = input[1].worldPos;
	
	triStream.Append(o);
	
	o.position = mul(input[2].position, proj);
	o.normal = input[2].normal;
	o.uv = input[2].uv;
	o.lightSpacePos = input[2].lightSpacePos;
	o.worldPos = input[2].worldPos;
	
	triStream.Append(o);
	
	triStream.RestartStrip();
}
