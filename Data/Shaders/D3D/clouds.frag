#include "include/ubos.hlsli"

Texture3D baseNoiseTexture : register(index0);
SamplerState baseNoiseSampler : register(s1);

Texture3D highFreqNoiseTexture : register(index1);
SamplerState highFreqNoiseSampler : register(s2);

Texture2D weatherTexture : register(index2);
SamplerState weatherSampler : register(s3);

struct PixelInput
{
	float4 position : SV_POSITION;
	float2 uv : TEXCOORD0;
	float3 camRay : TEXCOORD1;
};

static const float4 stratus = float4(0.0, 0.05, 0.1, 0.2);
static const float2 cumulus = float2(0.0, 0.45);
static const float2 cumulonimbus = float2(0.0, 1.0);

static const float planetSize = 350000.0;
static const float3 planetCenter = float3(0.0, -planetSize, 0.0);
static const float maxSteps = 128.0;
static const float minSteps = 64.0;
static const float baseScale = 0.00006;

float3 raySphere(float3 sc, float sr, float3 ro, float3 rd)
{
    float3 oc = ro - sc;
    float b = dot(rd, oc);
    float c = dot(oc, oc) - sr*sr;
    float t = b*b - c;
    if( t > 0.0) 
        t = -b - sqrt(t);
    return ro + (c/t) * rd;
}

uint calcRaySphereIntersection(float3 rayOrigin, float3 rayDirection, float3 sphereCenter, float radius, out float2 t)
{
	float3 l = rayOrigin - sphereCenter;
	float a = 1.0;
	float b = 2.0 * dot(rayDirection, l);
	float c = dot(l, l) - radius * radius;
	float discriminant = b * b - 4.0 * a * c;
	if (discriminant < 0.0)
	{
		t.x = t.y = 0.0;
		return 0;
	}
	else if (abs(discriminant) - 0.00005 <= 0.0)
	{
		t.x = t.y = -0.5 * b / a;
		return 1;
	}
	else
	{
		float q = b > 0.0 ? -0.5 * (b + sqrt(discriminant)) : -0.5 * (b - sqrt(discriminant));

		float h1 = q / a;
		float h2 = c / q;
		t.x = min(h1, h2);
		t.y = max(h1, h2);
		if (t.x < 0.0)
		{
			t.x = t.y;
			if (t.x < 0.0)
			{
				return 0;
			}
			return 1;
		}
		return 2;
	}
}

float remap(float original_value, float original_min, float original_max, float new_min, float new_max)
{
	return new_min + (((original_value - original_min) / (original_max - original_min)) * (new_max - new_min));
}

float beerTerm(float density)
{
	return exp(-density * densityMult);
}

float powderEffect(float density, float cosAngle)
{
	return 1.0 - exp(-density * 2.0 * densityMult);
}

float HenyeyGreensteinPhase(float cosAngle, float g)
{
	float g2 = g * g;
	return ((1.0 - g2) / pow(1.0 + g2 - 2.0 * g * cosAngle, 1.5)) / 4.0 * 3.1415;
}

float SampleNoise(float3 pos, float coverage, float cloudType, float height_0to1)
{
	pos.y *= baseScale;
	pos.x = pos.x * baseScale + timeElapsed * timeScale;
	pos.z = pos.z * baseScale - timeElapsed * timeScale;
	pos.y -= timeElapsed * timeScale * 0.3;																				// Vertical motion
	//pos += height_0to1 * height_0to1 * height_0to1 * normalize(float3(1.0, 0.0, 1.0)) * 0.3 * cloudType;				// Shear
	
	float height_1to0 = 1.0 - height_0to1;
	float4 noise = baseNoiseTexture.SampleLevel(baseNoiseSampler, pos, 0.0);
	
	//float verticalCoverage = textureLod(verticalCoverageTexture, float2(cloudType, 1.0-height_0to1), 0).r;  // y is flipped
	
	float lowFreqFBM = noise.g * 0.625 + noise.b * 0.25 + noise.a * 0.125;
	float cloud = noise.r * lowFreqFBM;
	cloud *= coverage;
	cloud *= cloudCoverage;
	//cloud *= remap(height_0to1, stratus.x, stratus.y, 0.0, 1.0) * remap(height_0to1, stratus.z, stratus.w, 1.0, 0.0);
	
	//cloud *= height_0to1  * 16.0 * coverage * cloudCoverage;
	
	float3 detailCoord = pos * detailScale;	
	float3 highFreqNoise = baseNoiseTexture.SampleLevel(baseNoiseSampler, detailCoord, 0.0).rgb;
	float highFreqFBM = highFreqNoise.r * 0.625 + highFreqNoise.g * 0.25 + highFreqNoise.b * 0.125;

	cloud = remap(cloud, 1.0 - highFreqFBM * height_1to0, 1.0, 0.0, 1.0);							// Erode the edges of the clouds. Multiply by height_1to0 to erode more at the bottom and preserve the tops
	cloud +=  12.0 * cloudCoverage * coverage * cloudCoverage;
	cloud *= smoothstep(0.0, 0.1, height_0to1);					// Smooth the bottoms
	//cloud -= smoothstep(0.0, 0.25, height_0to1);				// More like elevated convection
	//cloud *= verticalCoverage;
	
	cloud = saturate(cloud);

	return cloud;
}

float2 sampleWeather(float3 pos)
{
	pos.x = pos.x * 0.000045 + timeElapsed * timeScale;
	pos.z = pos.z * 0.000045 - timeElapsed * timeScale;
	return weatherTexture.SampleLevel(weatherSampler, pos.xz, 0.0).rg;
}

float getNormalizedHeight(float3 pos)
{
	return (distance(pos,  planetCenter) - (planetSize + cloudStartHeight)) / cloudLayerThickness;
}

float4 clouds(float3 dir)
{
	float3 rayStart = float3(0.0,0.0,0.0);
	float3 rayEnd = float3(0.0,0.0,0.0);
	float3 rayPos = float3(0.0,0.0,0.0);
	
	//rayStart  = raySphere(planetCenter, planetSize + cloudStartHeight, camPos.xyz, dir);
	//rayEnd = raySphere(planetCenter, planetSize + cloudLayerTopHeight, camPos.xyz, dir);
	//rayEnd = InternalRaySphereIntersect(planetSize + cloudLayerTopHeight, camPos.xyz, dir);

	float distanceCamPlanet = distance(camPos.xyz, planetCenter);
	
	float2 ih = float2(0.0,0.0);
	float2 oh = float2(0.0,0.0);
	
	uint innerShellHits = calcRaySphereIntersection(camPos.xyz, dir, planetCenter, planetSize + cloudStartHeight, ih);
	uint outerShellHits = calcRaySphereIntersection(camPos.xyz, dir, planetCenter, planetSize + cloudLayerTopHeight, oh);
	
	float3 innerHit = camPos.xyz + dir * ih.x;
	float3 outerHit = camPos.xyz + dir * oh.x;
	
	// Below the cloud layer
	if (distanceCamPlanet < planetSize + cloudStartHeight)
	{
		rayStart = innerHit;
		// Don't march if the ray is below the horizon
		if(rayStart.y < 0.0)
			return float4(0.0,0.0,0.0,0.0);
			
		rayEnd = outerHit;
	}
	// Above the cloud layer
	/*else if (distanceCamPlanet > planetSize + cloudLayerTopHeight)
	{
		// We can enter the outer shell and leave through the inner shell or enter the outer shell and leave through the outer shell
		rayStart = outerHit;
		// Don't march iif we don't hit the outer shell
		if (outerShellHits == 0)
			return float4(0.0,0.0,0.0,0.0);
		rayEnd = outerShellHits == 2 && innerShellHits == 0 ? camPos.xyz + dir * oh.y : innerHit;
	}
	// In the cloud layer
	else
	{
		rayStart = camPos.xyz;
		rayEnd = innerShellHits > 0 ? innerHit : outerHit;
	}*/
	
	float steps = int(lerp(maxSteps, minSteps, dir.y));		// Take more steps when the ray is pointing more towards the horizon
	float stepSize = distance(rayStart, rayEnd) / steps;
	
	const float largeStepMult = 3.0;
	float stepMult = 1.0;
	
	rayPos = rayStart;
	
	const float cosAngle = dot(dir, dirAndIntensity.xyz);
	float4 result = float4(0.0,0.0,0.0,0.0);
	
	for (float i = 0.0; i < steps; i += stepMult)
	{
		if (result.a >= 0.99)
			break;
			
		float heightFraction = getNormalizedHeight(rayPos);
		
		float2 weatherData = sampleWeather(rayPos);
		float density = SampleNoise(rayPos, weatherData.r, weatherData.g, heightFraction);
		
		if (density > 0.0)
		{		
			float height_0to1_Light = 0.0;
			float densityAlongLight = 0.0;
			
			float3 rayStep = dirAndIntensity.xyz * 40.0;
			float3 pos  = rayPos + rayStep;
			
			float thickness = 0.0;
			float scale = 1.0;
			
			for (int s = 0; s < 5; s++)
			{
				pos += rayStep * scale;
				float2 weatherData = sampleWeather(pos);
				height_0to1_Light = getNormalizedHeight(pos);
				densityAlongLight = SampleNoise(pos, weatherData.r, weatherData.g, height_0to1_Light);
				densityAlongLight *= float(height_0to1_Light <= 1.0);
				thickness += densityAlongLight;
				scale *= 4.0;
			}
			
			float direct = beerTerm(thickness) * powderEffect(density, cosAngle);
			//float HG = lerp(HenyeyGreensteinPhase(cosAngle, hgForward) , HenyeyGreensteinPhase(cosAngle, hgBackward), 0.5);		// To make facing away from the sun more interesting
			float hg = max(HenyeyGreensteinPhase(cosAngle, hgForward) * forwardSilverLiningIntensity, silverLiningIntensity * HenyeyGreensteinPhase(cosAngle, 0.99 - silverLiningSpread));
			direct *= hg * directLightMult;
		
			float3 ambient = lerp(ambientBottomColor.rgb, ambientTopColor.rgb, heightFraction) * ambientMult;
			
			float4 lighting = float4(density,density,density,density);
			lighting.rgb = direct * dirLightColor.xyz + ambient;
			lighting.rgb *= lighting.a;
	
			result = lighting * (1.0 - result.a) + result;
		}
		
		rayPos += stepSize * dir * stepMult;
	}
	
	// Add high clouds
	/*uint highCloudHits = calcRaySphereIntersection(camPos.xyz, dir, planetCenter, planetSize + cloudLayerTopHeight + 2000.0, oh);
	rayPos = camPos.xyz + dir * oh.x;
	rayPos.x = rayPos.x * 0.00005;
	rayPos.z = rayPos.z * 0.00005 + timeElapsed * highCloudsTimeScale;
	float highClouds = textureLod(highCloudsTexture, rayPos.xz, 0).r;
	highClouds *= highClouds * highClouds;
	highClouds *= highCloudsCoverage;
	
	highClouds *= float(rayPos.y > 0.0);
	
	float HG = lerp(HenyeyGreensteinPhase(cosAngle, hgForward) * 0.3 , HenyeyGreensteinPhase(cosAngle, hgBackward), 0.5) ;
	float3 col = highClouds * HG * dirLightColor.xyz * directLightMult;
	
	result.rgb = col * (1.0 - result.a) + result.rgb;
	*/
	return result;
}

/*float SampleRain(float3 pos, float coverage, float cloudType)
{
	return coverage * 0.5 * coverage * cloudCoverage * cloudCoverage;
}

float4 rain(float3 dir)
{
	float3 rayPos = float3(0.0);
	float3 rayEnd = float3(0.0);
	
	float distanceCamPlanet = distance(camPos.xyz, planetCenter);
	float2 ih = float2(0.0);
	float2 oh = float2(0.0);
	uint innerShellHits = calcRaySphereIntersection(camPos.xyz, dir, planetCenter, planetSize, ih);
	uint outerShellHits = calcRaySphereIntersection(camPos.xyz, dir, planetCenter, planetSize + cloudStartHeight, oh);
	
	rayPos = camPos.xyz + dir * ih.x;
	rayEnd = camPos.xyz + dir * oh.x;
	
	if(rayPos.y < 0.0)
		return float4(0.0,0.0,0.0,0.0);
			
	const int steps = 8;
	const float stepSize = distance(rayPos, rayEnd) / steps;
	
	//const float3 lightPos = float3(0.0, 2000.0, 0.0);
	//const float cosAngle = dot(dir, dirAndIntensity.xyz);
	float4 res = float4(0.0,0.0,0.0,0.0);
	
	for (int i = 0; i < steps; i++)
	{
		if (res.a >= 0.99)
			break;
			
		if(rayPos.y < 0.0)
			break;
			
		float2 weatherData = sampleWeather(rayPos);		
		float density = SampleRain(rayPos, weatherData.r, weatherData.g);
	//	if(density>0.0)
			res = (1.0 - res.a) * float4(density, density, density, clamp(density*5.0,0.0,1.0))  + res;
		
		rayPos += stepSize * dir;
	}
	res.rgb *=  ambientBottomColor;
	return res;
}*/

float4 PS(PixelInput i) : SV_TARGET
{
	float3 dir = normalize(i.camRay);
	float4 color =  clouds(dir);
	
	//float a = smoothstep(0.1, 1.0, clamp(dir.y * 8.0, 0.0, 1.0));
	//color = lerp(float4(0.0,0.0,0.0,0.0), color, a);
	//float4 rain = rain(dir);
	//color = color * (1.0 - rain.a) + rain;
//	color = lerp(color, float4(0.0), clamp(-dir.y * 20.0 + 0.9, 0.0, 1.0));

	return color;
}