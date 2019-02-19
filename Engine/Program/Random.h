#pragma once

#include "include\glm\glm.hpp"

#include <random>

namespace Engine
{
	class Random
	{
	public:
		static void Init();

		static float Value1DSharp(float x, float frequency);
		static float Value1DSmooth(float x, float frequency);
		static float Value2DSharp(float x, float y, float frequency);
		static float Value2DSmooth(float x, float y, float frequency);
		static float Value3DSharp(float x, float y, float z, float frequency);
		static float Value3DSmooth(float x, float y, float z, float frequency);
		static float Perlin1D(float x, float frequency);
		static float Perlin2D(float x, float y, float frequency);
		static float Perlin3D(float x, float y, float z, float frequency, int period);
		static float Perlin2D(float x, float y, float frequency, float octaves, float lacunarity, float persistence);
		static float Perlin3D(float x, float y, float z, float frequency, float octaves, float lacunarity, float persistence);
		static float WorleyNoise(const glm::vec3 &point, float cellCount);

		// Returns a random float between 0 and 1
		static float Float();
		// Returns a random float between low and high
		static float Float(float low, float high);
		// Returns a random int between 0 and 1
		static int Int();
		// Returns a random int between low and high
		static int Int(int low, int high);

	private:
		static float Smooth(float t);
		static float WorleyDistance(const glm::vec3 &point, const glm::vec3 &worleyPoint, float maxDist);
		static float noise(const glm::vec3 &x);
		static float Hash(float n);

	private:
		static int hash[512];
		static float gradient1D[2];
		static glm::vec2 gradient2D[4];
		static glm::vec3 gradient3D[16];
		static std::random_device rd;
		static std::mt19937 mt;
		static std::uniform_real_distribution<float> dist;
		static std::uniform_int_distribution<int> intDist;
		static glm::vec3 worleyPoints[8];
	};
}
