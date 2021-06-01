#pragma once

#include "Physics\BoundingVolumes.h"
#include "TerrainData.h"
#include "Graphics\UniformBufferTypes.h"

namespace Engine
{
	class Camera;
	class Game;

	class TerrainNode
	{
	public:
		TerrainNode(int x, int z, int size, int level, float *heights, int resolution);
		~TerrainNode();

		//bool InsertVegetation(const Vegetation &newVeg, const glm::mat4 &m, float scale);

		bool LODSelect(float *lodVisRanges, int lodLevel, Camera *camera, std::vector<TerrainInstanceData> &data);
		bool LODSelectDebug(float* lodVisRanges, int lodLevel, Camera* camera, Game *game);
		void RecalculateNode(int x, int z, int size, int level, float *heights, int resolution);
		void UpdateHeights(float* heights, const glm::vec2& hitPos, float halfSize, int terrainResolution);

		unsigned short GetX() const { return x; }
		unsigned short GetZ() const { return z; }
		unsigned short GetSize() const { return size; }
		float GetMinHeight() const { return minHeight; }
		float GetMaxHeight() const { return maxHeight; }

	private:
		bool InBounds(int x, int z, int resolution);
		bool InSphere(float radius, const glm::vec3 &position);
		bool InFrustum(Camera *camera);
		bool ContainsPoint(const glm::vec2& point);
		float GetVal(float* heights, int x, int z, int terrainResolution);

	private:
		TerrainNode *topLeft;
		TerrainNode *topRight;
		TerrainNode *bottomLeft;
		TerrainNode *bottomRight;
		unsigned short x;
		unsigned short z;
		float minHeight;
		float maxHeight;
		unsigned short level;
		unsigned short size;
		//AABB aabbVeg;
		//std::vector<Vegetation> veg;
		//std::vector<ModelInstanceData> vegData;
	};
}
