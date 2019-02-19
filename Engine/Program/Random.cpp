#include "Random.h"

namespace Engine
{
	int Random::hash[512] = {
		151,160,137, 91, 90, 15,131, 13,201, 95, 96, 53,194,233,  7,225,
		140, 36,103, 30, 69,142,  8, 99, 37,240, 21, 10, 23,190,  6,148,
		247,120,234, 75,  0, 26,197, 62, 94,252,219,203,117, 35, 11, 32,
		57,177, 33, 88,237,149, 56, 87,174, 20,125,136,171,168, 68,175,
		74,165, 71,134,139, 48, 27,166, 77,146,158,231, 83,111,229,122,
		60,211,133,230,220,105, 92, 41, 55, 46,245, 40,244,102,143, 54,
		65, 25, 63,161,  1,216, 80, 73,209, 76,132,187,208, 89, 18,169,
		200,196,135,130,116,188,159, 86,164,100,109,198,173,186,  3, 64,
		52,217,226,250,124,123,  5,202, 38,147,118,126,255, 82, 85,212,
		207,206, 59,227, 47, 16, 58, 17,182,189, 28, 42,223,183,170,213,
		119,248,152,  2, 44,154,163, 70,221,153,101,155,167, 43,172,  9,
		129, 22, 39,253, 19, 98,108,110, 79,113,224,232,178,185,112,104,
		218,246, 97,228,251, 34,242,193,238,210,144, 12,191,179,162,241,
		81, 51,145,235,249, 14,239,107, 49,192,214, 31,181,199,106,157,
		184, 84,204,176,115,121, 50, 45,127,  4,150,254,138,236,205, 93,
		222,114, 67, 29, 24, 72,243,141,128,195, 78, 66,215, 61,156,180,

		151,160,137, 91, 90, 15,131, 13,201, 95, 96, 53,194,233,  7,225,
		140, 36,103, 30, 69,142,  8, 99, 37,240, 21, 10, 23,190,  6,148,
		247,120,234, 75,  0, 26,197, 62, 94,252,219,203,117, 35, 11, 32,
		57,177, 33, 88,237,149, 56, 87,174, 20,125,136,171,168, 68,175,
		74,165, 71,134,139, 48, 27,166, 77,146,158,231, 83,111,229,122,
		60,211,133,230,220,105, 92, 41, 55, 46,245, 40,244,102,143, 54,
		65, 25, 63,161,  1,216, 80, 73,209, 76,132,187,208, 89, 18,169,
		200,196,135,130,116,188,159, 86,164,100,109,198,173,186,  3, 64,
		52,217,226,250,124,123,  5,202, 38,147,118,126,255, 82, 85,212,
		207,206, 59,227, 47, 16, 58, 17,182,189, 28, 42,223,183,170,213,
		119,248,152,  2, 44,154,163, 70,221,153,101,155,167, 43,172,  9,
		129, 22, 39,253, 19, 98,108,110, 79,113,224,232,178,185,112,104,
		218,246, 97,228,251, 34,242,193,238,210,144, 12,191,179,162,241,
		81, 51,145,235,249, 14,239,107, 49,192,214, 31,181,199,106,157,
		184, 84,204,176,115,121, 50, 45,127,  4,150,254,138,236,205, 93,
		222,114, 67, 29, 24, 72,243,141,128,195, 78, 66,215, 61,156,180
	};

	float Random::gradient1D[2] = { 1.0f, -1.0f };
	glm::vec2 Random::gradient2D[4] = { 
		glm::vec2(1.0f, 0.0f),
		glm::vec2(-1.0f, 0.0f),
		glm::vec2(0.0f, 1.0f),
		glm::vec2(0.0f, -1.0f)
	};
	glm::vec3 Random::gradient3D[16] = { 
		glm::vec3(1.0f, 1.0f, 0.0f),
		glm::vec3(-1.0f, 1.0f, 0.0f),
		glm::vec3(1.0f, -1.0f, 0.0f),
		glm::vec3(-1.0f, -1.0f, 0.0f),

		glm::vec3(1.0f, 0.0f, 1.0f),
		glm::vec3(-1.0f, 0.0f, 1.0f),
		glm::vec3(1.0f, 0.0f, -1.0f),
		glm::vec3(-1.0f, 0.0f, -1.0f),

		glm::vec3(0.0f, 1.0f, 1.0f),
		glm::vec3(0.0f, -1.0f, 1.0f),
		glm::vec3(0.0f, 1.0f, -1.0f),
		glm::vec3(0.0f, -1.0f, -1.0f),

		glm::vec3(1.0f, 1.0f, 0.0f),
		glm::vec3(-1.0f, 1.0f, 0.0f),
		glm::vec3(0.0f, -1.0f, 1.0f),
		glm::vec3(0.0f, -1.0f, -1.0f),
	};


	std::random_device Random::rd;
	std::mt19937 Random::mt;
	std::uniform_real_distribution<float> Random::dist;
	std::uniform_int_distribution<int> Random::intDist;
	glm::vec3 Random::worleyPoints[8];

	void Random::Init()
	{
		mt.seed(rd());
		dist.param(std::uniform_real_distribution<float>::param_type(0.0f, 1.0f));
		intDist.param(std::uniform_int_distribution<int>::param_type(0, 1));

		worleyPoints[0] = glm::vec3(0.2f, 0.5f, 0.1f);
		worleyPoints[1] = glm::vec3(0.8f, 0.3f, 0.4f);
		worleyPoints[2] = glm::vec3(0.1f, 0.4f, 0.7f);
		worleyPoints[3] = glm::vec3(0.7f, 0.2f, 0.3f);
		worleyPoints[4] = glm::vec3(0.9f, 0.3f, 0.6f);
		worleyPoints[5] = glm::vec3(0.6f, 0.1f, 0.8f);
		worleyPoints[6] = glm::vec3(0.5f, 0.7f, 0.2f);
		worleyPoints[7] = glm::vec3(0.25f, 0.9f, 0.9f);
	}

	float Random::Value1DSharp(float x, float frequency)
	{
		x *= frequency;
		int i = (int)std::floor(x);
		i &= 255;							// For i to wrap around
		return hash[i] / (1.0f / 255.0f);
	}

	float Random::Value1DSmooth(float x, float frequency)
	{
		x *= frequency;
		int iLeft = (int)std::floor(x);
		float t = x - iLeft;				// Distance from the sample point x to the left coordinate
		iLeft &= 255;
		int iRight = iLeft + 1;

		int h0 = hash[iLeft];
		int h1 = hash[iRight];

		t = Smooth(t);
		return glm::mix(h0, h1, t) * (1.0f / 255.0f);
	}

	float Random::Value2DSharp(float x, float y, float frequency)
	{
		x *= frequency;
		y *= frequency;
		int ix = (int)std::floor(x);
		int iy = (int)std::floor(y);
		ix &= 255;
		iy &= 255;
		return hash[hash[ix] + iy] / (1.0f / 255.0f);
	}

	float Random::Value2DSmooth(float x, float y, float frequency)
	{
		x *= frequency;
		y *= frequency;
		int ix0 = (int)std::floor(x);
		int iy0 = (int)std::floor(y);
		float tx = x - ix0;
		float ty = y - iy0;
		ix0 &= 255;
		iy0 &= 255;
		
		int ix1 = ix0 + 1;
		int iy1 = iy0 + 1;

		int h0 = hash[ix0];
		int h1 = hash[ix1];
		int h00 = hash[h0 + iy0];		// Bottom left
		int h10 = hash[h1 + iy0];		// Bottom right
		int h01 = hash[h0 + iy1];		// Top left
		int h11 = hash[h1 + iy1];		// Top right

		tx = Smooth(tx);
		ty = Smooth(ty);
		return glm::mix(glm::mix(h00, h10, tx), glm::mix(h01, h11, tx), ty) * (1.0f / 255.0f);
	}

	float Random::Value3DSharp(float x, float y, float z, float frequency)
	{
		x *= frequency;
		y *= frequency;
		z *= frequency;
		int ix = (int)std::floor(x);
		int iy = (int)std::floor(y);
		int iz = (int)std::floor(z);
		ix &= 255;
		iy &= 255;
		iz &= 255;
		return hash[hash[hash[ix] + iy] + iz] * (1.0f / 255.0f);
	}

	float Random::Value3DSmooth(float x, float y, float z, float frequency)
	{
		x *= frequency;
		y *= frequency;
		z *= frequency;
		int ix0 = (int)std::floor(x);
		int iy0 = (int)std::floor(y);
		int iz0 = (int)std::floor(z);
		float tx = x - ix0;
		float ty = y - iy0;
		float tz = z - iz0;
		ix0 &= 255;
		iy0 &= 255;
		iz0 &= 255;

		int ix1 = ix0 + 1;
		int iy1 = iy0 + 1;
		int iz1 = iz0 + 1;

		int h0 = hash[ix0];
		int h1 = hash[ix1];

		int h00 = hash[h0 + iy0];
		int h10 = hash[h1 + iy0];
		int h01 = hash[h0 + iy1];
		int h11 = hash[h1 + iy1];

		int h000 = hash[h00 + iz0];
		int h100 = hash[h10 + iz0];
		int h010 = hash[h01 + iz0];
		int h110 = hash[h11 + iz0];
		int h001 = hash[h00 + iz1];
		int h101 = hash[h10 + iz1];
		int h011 = hash[h01 + iz1];
		int h111 = hash[h11 + iz1];

		tx = Smooth(tx);
		ty = Smooth(ty);
		tz = Smooth(tz);
		return glm::mix(
			glm::mix(glm::mix(h000, h100, tx), glm::mix(h010, h110, tx), ty),
			glm::mix(glm::mix(h001, h101, tx), glm::mix(h011, h111, tx), ty),
			tz) * (1.0f / 255.0f);
	}

	float Random::Perlin1D(float x, float frequency)
	{
		x *= frequency;
		int i0 = (int)std::floor(x);
		float t0 = x - i0;
		float t1 = t0 - 1.0f;
		i0 &= 255;
		int i1 = i0 + 1;

		float g0 = gradient1D[hash[i0] & 1];
		float g1 = gradient1D[hash[i1] & 1];

		float v0 = g0 * t0;
		float v1 = g1 * t1;

		float t = Smooth(t0);
		return glm::mix(v0, v1, t);
	}

	float Random::Perlin2D(float x, float y, float frequency)
	{
		x *= frequency;
		y *= frequency;
		int ix0 = (int)std::floor(x);
		int iy0 = (int)std::floor(y);

		glm::vec2 unitSquarePos = glm::vec2(x - ix0, y - iy0);

		float tx1 = unitSquarePos.x - 1.0f;
		float ty1 = unitSquarePos.y - 1.0f;

		ix0 &= 255;
		iy0 &= 255;

		int ix1 = ix0 + 1;
		int iy1 = iy0 + 1;

		int h0 = hash[ix0];
		int h1 = hash[ix1];

		glm::vec2 g00 = gradient2D[hash[h0 + iy0] & 3];
		glm::vec2 g10 = gradient2D[hash[h1 + iy0] & 3];
		glm::vec2 g01 = gradient2D[hash[h0 + iy1] & 3];
		glm::vec2 g11 = gradient2D[hash[h1 + iy1] & 3];

		// Now take the dot product between the gradient vectors and distnace vectors
		float v00 = g00.x * unitSquarePos.x + g00.y * unitSquarePos.y;
		float v10 = g10.x * tx1 + g10.y * unitSquarePos.y;
		float v01 = g01.x * unitSquarePos.x + g01.y * ty1;
		float v11 = g11.x * tx1 + g11.y * ty1;

		float tx = Smooth(unitSquarePos.x);
		float ty = Smooth(unitSquarePos.y);

		float x1 = glm::mix(v00, v10, tx);
		float x2 = glm::mix(v01, v11, tx);

		return glm::mix(x1, x2, ty);
	}

	float Random::Perlin3D(float x, float y, float z, float frequency, int period)
	{
		x *= frequency;
		y *= frequency;
		z *= frequency;
		int ix0 = (int)std::floor(x);
		int iy0 = (int)std::floor(y);
		int iz0 = (int)std::floor(z);

		glm::vec3 unitCubePos = glm::vec3(x - ix0, y - iy0, z - iz0);

		float tx1 = unitCubePos.x - 1.0f;
		float ty1 = unitCubePos.y - 1.0f;
		float tz1 = unitCubePos.z - 1.0f;

		int ix1 = ((ix0 + 1) % period) & 255;
		int iy1 = ((iy0 + 1) % period) & 255;
		int iz1 = ((iz0 + 1) % period) & 255;

		ix0 = (ix0 % period) & 255;
		iy0 = (iy0 % period) & 255;
		iz0 = (iz0 % period) & 255;

		int h0 = hash[ix0];
		int h1 = hash[ix1];

		int h00 = hash[h0 + iy0];
		int h10 = hash[h1 + iy0];
		int h01 = hash[h0 + iy1];
		int h11 = hash[h1 + iy1];

		glm::vec3 g000 = gradient3D[hash[h00 + iz0] & 15];
		glm::vec3 g100 = gradient3D[hash[h10 + iz0] & 15];
		glm::vec3 g010 = gradient3D[hash[h01 + iz0] & 15];
		glm::vec3 g110 = gradient3D[hash[h11 + iz0] & 15];
		glm::vec3 g001 = gradient3D[hash[h00 + iz1] & 15];
		glm::vec3 g101 = gradient3D[hash[h10 + iz1] & 15];
		glm::vec3 g011 = gradient3D[hash[h01 + iz1] & 15];
		glm::vec3 g111 = gradient3D[hash[h11 + iz1] & 15];

		// Now take the dot product between the gradient vectors and distnace vectors
		float v000 = g000.x * unitCubePos.x + g000.y * unitCubePos.y + g000.z * unitCubePos.z;
		float v100 = g100.x * tx1 + g100.y * unitCubePos.y + g100.z * unitCubePos.z;
		float v010 = g010.x * unitCubePos.x + g010.y * ty1 + g010.z * unitCubePos.z;
		float v110 = g110.x * tx1 + g110.y * ty1 + g110.z * unitCubePos.z;
		float v001 = g001.x * unitCubePos.x + g001.y * unitCubePos.y + g001.z * tz1;
		float v101 = g101.x * tx1 + g101.y * unitCubePos.y + g101.z * tz1;
		float v011 = g011.x * unitCubePos.x + g011.y * ty1 + g011.z * tz1;
		float v111 = g111.x * tx1 + g111.y * ty1 + g111.z * tz1;

		float tx = Smooth(unitCubePos.x);
		float ty = Smooth(unitCubePos.y);
		float tz = Smooth(unitCubePos.z);

		float x1 = glm::mix(v000, v100, tx);
		float x2 = glm::mix(v010, v110, tx);
		float x3 = glm::mix(v001, v101, tx);
		float x4 = glm::mix(v011, v111, tx);

		float y1 = glm::mix(x1, x2, ty);
		float y2 = glm::mix(x3, x4, ty);

		return glm::mix(y1, y2, tz);
	}

	float Random::Perlin2D(float x, float y, float frequency, float octaves, float lacunarity, float persistence)
	{
		float sum = Perlin2D(x, y, frequency);
		float amplitude = 1.0f;
		float range = 1.0f;

		for (int i = 1; i < octaves; i++)
		{
			frequency *= lacunarity;
			amplitude *= persistence;
			range += amplitude;
			sum += Perlin2D(x, y, frequency);
		}

		float noise = (sum / range) * 0.5f + 0.5f;
		return glm::clamp(noise, 0.0f, 1.0f);
	}

	float Random::Perlin3D(float x, float y, float z, float frequency, float octaves, float lacunarity, float persistence)
	{
		int period = 4;
		float sum = Perlin3D(x, y, z, frequency, period);
		float amplitude = 1.0f;
		float range = 1.0f;

		for (int i = 1; i < octaves; i++)
		{
			frequency *= lacunarity;
			period *= (int)lacunarity;
			amplitude *= persistence;
			range += amplitude;
			sum += Perlin3D(x, y, z, frequency, period);
		}

		float noise = (sum / range) * 0.5f + 0.5f;
		return glm::clamp(noise, 0.0f, 1.0f);
	}

	float Random::WorleyDistance(const glm::vec3 &point, const glm::vec3 &worleyPoint, float maxDist)
	{
		float value = maxDist;

		for (float z = -1.0f; z < 1.1f; z += 1.0f)
		{
			for (float y = -1.0f; y < 1.1f; y += 1.0f)
			{
				for (float x = -1.0f; x < 1.1f; x += 1.0f)
				{
					glm::vec3 delta = worleyPoint - point + glm::vec3(x, y, z);
					value = std::min(value, glm::length(delta));
				}
			}
		}
		value = glm::clamp(value, 0.0f, 1.0f);
		return value;
	}

	float Random::WorleyNoise(const glm::vec3 & point, float cellCount)
	{
		/*float value = maxDistance;

		for (int i = 0; i < 8; i++)
		{
		value = WorleyDistance(point, worleyPoints[i], value);
		}
		return value / maxDistance;*/

		const glm::vec3 pCell = point * cellCount;
		float d = 1.0e10;
		for (int z = -1; z <= 1; z++)
		{
			for (int y = -1; y <= 1; y++)
			{
				for (int x = -1; x <= 1; x++)
				{
					glm::vec3 tp = glm::floor(pCell) + glm::vec3(x, y, z);
					tp = pCell - tp - noise(glm::mod(tp, cellCount));
					d = glm::min(d, glm::dot(tp, tp));
				}
			}
		}
		return glm::clamp(d, 0.0f, 1.0f);
	}

	float Random::Float()
	{
		return dist(mt);
	}

	float Random::Float(float low, float high)
	{
		return low + dist(mt) * (high - low);
	}

	int Random::Int()
	{
		return intDist(mt);
	}

	int Random::Int(int low, int high)
	{
		return low + intDist(mt) * (high - low);
	}

	float Random::Smooth(float t)
	{
		return t * t * t * (t * (t * 6.0f - 15.0f) + 10.0f);		// 6t^5 - 15t^4 + 10t^3
	}

	float Random::noise(const glm::vec3 &x)
	{
		glm::vec3 p = glm::floor(x);
		glm::vec3 f = glm::fract(x);

		f = f * f*(glm::vec3(3.0f) - glm::vec3(2.0f) * f);
		float n = p.x + p.y * 57.0f + 113.0f * p.z;
		return glm::mix(
			glm::mix(
				glm::mix(Hash(n + 0.0f), Hash(n + 1.0f), f.x),
				glm::mix(Hash(n + 57.0f), Hash(n + 58.0f), f.x),
				f.y),
			glm::mix(
				glm::mix(Hash(n + 113.0f), Hash(n + 114.0f), f.x),
				glm::mix(Hash(n + 170.0f), Hash(n + 171.0f), f.x),
				f.y),
			f.z);
	}

	float Random::Hash(float n)
	{
		return (float)glm::fract(std::sin(n + 1.951) * 43758.5453);
	}
}
