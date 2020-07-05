#version 450
#include "include/ubos.glsl"

layout(location = 0) out vec4 outColor;
//layout(location = 1) out vec4 outNormal;

layout(location = 0) in vec3 worldPos;

tex2D_u(0) transmittanceTexture;
tex3D_u(1) inscatterTexture;

/*layout(binding = FIRST_SLOT) uniform sampler2D transmittanceTexture;
layout(binding = FIRST_SLOT + 1) uniform sampler3D inscatterTexture;*/

const float M_PI = 3.141592657;
const int RES_R = 32;
const int RES_MU = 128;
const int RES_MU_S = 32;
const int RES_NU = 8;
const float Rg = 6360000.0;
const float Rt = 6420000.0;
const float RL = 6421000.0;
const float HR = 8.0;
const float HM = 1.2;
const vec3 betaMSca = vec3(4e-3);
const vec3 betaMEx = betaMSca / 0.9;
const vec3 EARTH_POS = vec3(0.0, 6360010.0, 0.0);
vec3 extinction;

const float mieG = 0.9;
const vec3 betaR = vec3(0.0058 / 1000.0, 0.0135 / 1000.0, 0.0331 / 1000.0);
const float SUN_INTENSITY = 100.0;

vec3 hdr(vec3 L)
{
	L = L * 0.4;
	L.r = L.r < 1.413 ? pow(L.r * 0.38317, 1.0 / 2.2) : 1.0 - exp(-L.r);
	L.g = L.g < 1.413 ? pow(L.g * 0.38317, 1.0 / 2.2) : 1.0 - exp(-L.g);
	L.b = L.b < 1.413 ? pow(L.b * 0.38317, 1.0 / 2.2) : 1.0 - exp(-L.b);
	return L;
}

vec3 Transmittance(float r, float mu)
{
	// transmittance(=transparency) of atmosphere for infinite ray (r,mu)
	// (mu=cos(view zenith angle)), intersections with ground ignored
	float uR, uMu;
	uR = sqrt((r - Rg) / (Rt - Rg));
	uMu = atan((mu + 0.15) / (1.0 + 0.15) * tan(1.5)) / 1.5;

	return texture(transmittanceTexture, vec2(uMu, uR)).rgb;
}

vec3 GetMie(vec4 rayMie)
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

vec4 Texture4D(sampler3D table, float r, float mu, float muS, float nu)
{
	float H = sqrt(Rt * Rt - Rg * Rg);
	float rho = sqrt(r * r - Rg * Rg);

	float rmu = r * mu;
	float delta = rmu * rmu - r * r + Rg * Rg;
	vec4 cst = rmu < 0.0 && delta > 0.0 ? vec4(1.0, 0.0, 0.0, 0.5 - 0.5 / float(RES_MU)) : vec4(-1.0, H * H, H, 0.5 + 0.5 / float(RES_MU));
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
	return texture(table, vec3((uNu + uMuS) / float(RES_NU), uMu, uR)) * (1.0 - lep) + texture(table, vec3((uNu + uMuS + 1.0) / float(RES_NU), uMu, uR)) * lep;
}

vec3 SkyRadiance(vec3 camera, vec3 viewdir, vec3 sunDir)
{
	// scattered sunlight between two points
	// camera=observer
	// viewdir=unit vector towards observed point
	// sundir=unit vector towards the sun
	// return scattered light

	camera += EARTH_POS;

	vec3 result = vec3(0, 0, 0);
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

		vec4 inScatter = Texture4D(inscatterTexture, r, rMu / r, muS, nu);
		extinction = Transmittance(r, mu);

		vec3 inScatterM = GetMie(inScatter);
		float phase = PhaseFunctionR(nu);
		float phaseM = PhaseFunctionM(nu);
		result = inScatter.rgb * phase + inScatterM * phaseM;
	}
	else
	{
		result = vec3(0, 0, 0);
		extinction = vec3(1, 1, 1);
	}

	return result * SUN_INTENSITY;
}

/*float Hash( float n )
{
        return fract( (1.0 + sin(n)) * 415.92653);
}
float Noise3d( vec3 x )
{
    float xhash = Hash(round(400*x.x) * 37.0);
    float yhash = Hash(round(400*x.y) * 57.0);
    float zhash = Hash(round(400*x.z) * 67.0);
    return fract(xhash + yhash + zhash);
}*/

void main()
{
	vec3 dir = normalize(worldPos - camPos.xyz);
	
	float sun = step(cos(M_PI / 360.0), dot(dir, dirAndIntensity.xyz) * 1.001);		// Multiply to increase the sun disk a bit
	vec3 sunColor = vec3(sun, sun, sun) * SUN_INTENSITY;
	vec3 inscatter = SkyRadiance(camPos.xyz, dir, dirAndIntensity.xyz);
	outColor.rgb = sunColor * extinction + inscatter;
	outColor.rgb = hdr(outColor.rgb);
	outColor.rgb = pow(outColor.rgb, vec3(2.2));
	outColor.rgb += sunColor * extinction * 0.05;
	
	//outNormal = vec4(0.0);
	
	/*float star_intensity = Noise3d(dir);
    // And we apply a threshold to keep only the brightest areas
    if (star_intensity >= 0.99)
	{
        //We compute the star intensity
        star_intensity = pow((star_intensity - 0.99) / (1.0 - 0.99), 6.0)*(clamp(-dirAndIntensity.y+0.1, 0.0, 1.0));
        outColor.rgb += vec3(star_intensity);
    }*/
	
	outColor.a = 1.0;
}