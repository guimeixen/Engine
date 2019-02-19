#pragma once

#include "include\glm\glm.hpp"

#include <vector>

namespace Engine
{
	class Model;

	struct TerrainInstanceData
	{
		glm::vec4 params;
	};

	struct Vegetation
	{
		std::vector<std::string> matnames;
		float minScale = 0.0f;
		float maxScale = 0.0f;
		float maxSlope = 0.0f;
		int density;
		float heightOffset = 0.0f;
		int spacing = 0;
		Model *model;
		Model *modelLOD1;
		Model *modelLOD2;
		float lod1Dist;
		float lod2Dist;
		bool renderLOD0;
		bool renderLOD1;
		bool renderLOD2;
		unsigned int capacity;
		unsigned int count;
		unsigned int offset;
		float lodDist;
		bool generateColliders;
		bool generateObstacles;
	};
}
