#include "include/ubos.hlsli"

Texture2D transmittanceTexture : register(index0);
SamplerState transmittanceSampler : register(s1);
Texture3D inscatterTexture : register(index1);
SamplerState inscatterSampler : register(s2);

struct PixelInput
{
	float4 position : SV_POSITION;
	float3 worldPos : TEXCOORD1;
};

static const float M_PI = 3.141592657;
static const int RES_R = 32;
static const int RES_MU = 128;
static const int RES_MU_S = 32;
static const int RES_NU = 8;
static const float Rg = 6360000.0;
static const float Rt = 6420000.0;
static const float RL = 6421000.0;
static const float HR = 8.0;
static const float HM = 1.2;
static const float3 betaMSca = float3(4e-3, 4e-3, 4e-3);
static const float3 betaMEx = betaMSca / 0.9;
static const float3 EARTH_POS = float3(0.0, 6360010.0, 0.0);
static const float mieG = 0.9;
static const float3 betaR = float3(0.0058 / 1000.0, 0.0135 / 1000.0, 0.0331 / 1000.0);
static const float SUN_INTENSITY = 100.0;

float3 hdr(float3 L)
{
	L = L * 0.4;
	L.r = L.r < 1.413 ? pow(L.r * 0.38317, 1.0 / 2.2) : 1.0 - exp(-L.r);
	L.g = L.g < 1.413 ? pow(L.g * 0.38317, 1.0 / 2.2) : 1.0 - exp(-L.g);
	L.b = L.b < 1.413 ? pow(L.b * 0.38317, 1.0 / 2.2) : 1.0 - exp(-L.b);
	return L;
}

float3 Transmittance(float r, float mu)
{
	// transmittance(=transparency) of atmosphere for infinite ray (r,mu)
	// (mu=cos(view zenith angle)), intersections with ground ignored
	float uR, uMu;
	uR = sqrt((r - Rg) / (Rt - Rg));
	uMu = atan((mu + 0.15) / (1.0 + 0.15) * tan(1.5)) / 1.5;

	return transmittanceTexture.Sample(transmittanceSampler, float2(uMu, uR)).rgb;
}

float3 GetMie(float4 rayMie)
{
	// approximated single Mie scattering (cf. approximate Cm in paragraph "Angular precision")
	// rayMie.rgb=C*, rayMie.w=Cm,r
	return rayMie.rgb * rayMie.w / max(rayMie.r, 1e-4) * (betaR.r / betaR);
}

float PhaseFunctionR(float mu)
{
	// Rayleigh phase function
	return (3.0 / (16.0 * M_PI)) * (1.0 + mu * mu);
}

float PhaseFunctionM(float mu)
{
	// Mie phase function
	return 1.5 / (4.0 * M_PI) * (1.0 - mieG*mieG) * pow(1.0 + (mieG*mieG) - 2.0*mieG*mu, -3.0 / 2.0) * (1.0 + mu * mu) / (2.0 + mieG*mieG);
	//return 1.5 / (4.0 * M_PI) * (1.0 - mieG * mieG) * pow
}

float4 Texture4D(float r, float mu, float muS, float nu)
{
	float H = sqrt(Rt * Rt - Rg * Rg);
	float rho = sqrt(r * r - Rg * Rg);

	float rmu = r * mu;
	float delta = rmu * rmu - r * r + Rg * Rg;
	float4 cst = rmu < 0.0 && delta > 0.0 ? float4(1.0, 0.0, 0.0, 0.5 - 0.5 / float(RES_MU)) : float4(-1.0, H * H, H, 0.5 + 0.5 / float(RES_MU));
	float uR = 0.5 / float(RES_R) + rho / H * (1.0 - 1.0 / float(RES_R));
	float uMu = cst.w + (rmu * cst.x + sqrt(delta + cst.y)) / (rho + cst.z) * (0.5 - 1.0 / float(RES_MU));
	// paper formula
	//float uMuS = 0.5 / RES_MU_S + max((1.0 - exp(-3.0 * muS - 0.6)) / (1.0 - exp(-3.6)), 0.0) * (1.0 - 1.0 / RES_MU_S);
	// better formula
	float uMuS = 0.5 / float(RES_MU_S) + (atan(max(muS, -0.1975) * tan(1.26 * 1.1)) / 1.1 + (1.0 - 0.26)) * 0.5 * (1.0 - 1.0 / float(RES_MU_S));

	float lep = (nu + 1.0) / 2.0 * (float(RES_NU) - 1.0);
	float uNu = floor(lep);
	lep = lep - uNu;

	//Original 3D lookup
	return inscatterTexture.Sample(inscatterSampler, float3((uNu + uMuS) / float(RES_NU), uMu, uR)) * (1.0 - lep) + inscatterTexture.Sample(inscatterSampler, float3((uNu + uMuS + 1.0) / float(RES_NU), uMu, uR)) * lep;
}

float3 SkyRadiance(float3 camera, float3 viewdir, float3 sunDir, out float3 extinction)
{
	// scattered sunlight between two points
	// camera=observer
	// viewdir=unit vector towards observed point
	// sundir=unit vector towards the sun
	// return scattered light

	camera += EARTH_POS;

	float3 result = float3(0, 0, 0);
	float r = length(camera);
	float rMu = dot(camera, viewdir);
	float mu = rMu / r;
	float r0 = r;
	float mu0 = mu;

	float deltaSq = sqrt(rMu * rMu - r * r + Rt * Rt);
	float din = max(-rMu - deltaSq, 0.0);
	if (din > 0.0)
	{
		camera += din * viewdir;
		rMu += din;
		mu = rMu / Rt;
		r = Rt;
	}

	if (r <= Rt)
	{
		float nu = dot(viewdir, sunDir);
		float muS = dot(camera, sunDir) / r;

		float4 inScatter = Texture4D(r, rMu / r, muS, nu);
		extinction = Transmittance(r, mu);

		float3 inScatterM = GetMie(inScatter);
		float phase = PhaseFunctionR(nu);
		float phaseM = PhaseFunctionM(nu);
		result = inScatter.rgb * phase + inScatterM * phaseM;
	}
	else
	{
		result = float3(0, 0, 0);
		extinction = float3(1, 1, 1);
	}

	return result * SUN_INTENSITY;
}

float4 PS(PixelInput i) : SV_TARGET
{
	float3 dir = normalize(i.worldPos - camPos.xyz);
	
	float sun = step(cos(M_PI / 360.0), dot(dir, dirAndIntensity.xyz) * 1.001);		// Multiply to increase the sun disk a bit
	float3 sunColor = float3(sun, sun, sun) * SUN_INTENSITY;
	float3 extinction;
	float3 inscatter = SkyRadiance(camPos.xyz, dir, dirAndIntensity.xyz, extinction);
	float4 outColor = float4(1.0,1.0,1.0,1.0);
	outColor.rgb = sunColor * extinction + inscatter;
	outColor.rgb = hdr(outColor.rgb);
	outColor.rgb = pow(abs(outColor.rgb), float3(2.2,2.2,2.2));
	outColor.rgb += sunColor * extinction * 0.05;
	
	outColor.a = 1.0;
	
	return outColor;
}