#pragma once

#include "Physics\BoundingVolumes.h"
#include "TerrainData.h"
#include "Graphics\UniformBufferTypes.h"

namespace Engine
{
	class Camera;

	class TerrainNode
	{
	public:
		TerrainNode(int x, int z, int size, int level, float *heights, int resolution);
		~TerrainNode();

		//bool InsertVegetation(const Vegetation &newVeg, const glm::mat4 &m, float scale);

		bool LODSelect(float *lodVisRanges, int lodLevel, Camera *camera, std::vector<TerrainInstanceData> &data);

		void RecalculateNode(int x, int z, int size, int level, float *heights, int resolution);

	private:
		bool InBounds(int x, int z, int resolution);
		bool InSphere(float radius, const glm::vec3 &position);
		bool InFrustum(Camera *camera);

	private:
		TerrainNode * topLeft;
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
