#include "TerrainNode.h"

#include "Graphics\Camera\Camera.h"
#include "Graphics\Model.h"

#include "Program\Utils.h"

#include <algorithm>

namespace Engine
{
	TerrainNode::TerrainNode(int x, int z, int size, int level, float *heights, int resolution)
	{
		this->x = (unsigned short)x;
		this->z = (unsigned short)z;
		this->size = (unsigned short)size;
		this->level = (unsigned short)level;

		if (level == 0)
		{
			topLeft = nullptr;
			topRight = nullptr;
			bottomLeft = nullptr;
			bottomRight = nullptr;

			// This is the last LOD
			// Find the min/max heights at this patch of terrain
			minHeight = 99999.0f;

			for (int j = z; j < z + size; j++)
			{
				for (int i = x; i < x + size; i++)
				{
					if (InBounds(i, j, resolution))
					{
						float newval = heights[j * resolution + i];

						if (newval < minHeight)
						{
							minHeight = newval;
						}
					}
				}
			}

			maxHeight = -99999.0f;
			for (int j = z; j < z + size; j++)
			{
				for (int i = x; i < x + size; i++)
				{
					if (InBounds(i, j, resolution))
					{
						float newval = heights[j * resolution + i];

						if (newval > maxHeight)
						{
							maxHeight = newval;
						}
					}
				}
			}
		}
		else
		{
			int halfSize = size / 2;

			// Use a buffer with all the nodes and pass the buffer and last index to here so use here, otherwise we're creating this two times
			topLeft = new TerrainNode(x, z, halfSize, level - 1, heights, resolution);
			topRight = new TerrainNode(x + halfSize, z, halfSize, level - 1, heights, resolution);
			bottomLeft = new TerrainNode(x, z + halfSize, halfSize, level - 1, heights, resolution);
			bottomRight = new TerrainNode(x + halfSize, z + halfSize, halfSize, level - 1, heights, resolution);

			maxHeight = std::max(std::max(topLeft->maxHeight, topRight->maxHeight), std::max(bottomLeft->maxHeight, bottomRight->maxHeight));
			minHeight = std::min(std::min(topLeft->minHeight, topRight->minHeight), std::min(bottomLeft->minHeight, bottomRight->minHeight));
		}
	}

	TerrainNode::~TerrainNode()
	{
		if (topLeft)
			delete topLeft;
		if (topRight)
			delete topRight;
		if (bottomLeft)
			delete bottomLeft;
		if (bottomRight)
			delete bottomRight;
	}

	/*bool TerrainNode::InsertVegetation(const Vegetation &newVeg, const glm::mat4 &m, float scale)
	{
		AABB aabb = newVeg.model->GetOriginalAABB();
		aabb.min *= scale;
		aabb.max *= scale;
		aabb.min += glm::vec3(m[3]);
		aabb.max += glm::vec3(m[3]);
		// Aadd rotation and use utils:recomputeaabb

		// Check if the vegetation instance aabb is inside
		glm::vec3 min = glm::vec3(x, minHeight, z);
		glm::vec3 max = glm::vec3(x + size, maxHeight, z + size);

		bool intersect = utils::AABBABBBIntersection(aabb, { min, max });

		if (intersect)
		{
			bool lowestNode = true;
			if (topLeft)
			{
				topLeft->InsertVegetation(newVeg, m, scale);
				lowestNode = false;
			}
			if (topRight)
			{
				topRight->InsertVegetation(newVeg, m, scale);
				lowestNode = false;
			}
			if (bottomLeft)
			{
				topLeft->InsertVegetation(newVeg, m, scale);
				bottomLeft = false;
			}
			if (bottomRight)
			{
				bottomRight->InsertVegetation(newVeg, m, scale);
				lowestNode = false;
			}

			if(lowestNode)							// If we've reached the lowest node then insert the vegetation
			{
				bool found = false;
				for (size_t i = 0; i < veg.size(); i++)
				{
					Vegetation &v = veg[i];
					if (v.model == newVeg.model)		// Check if we already have this vegetation type
					{
						vegData.insert(vegData.begin() + veg[i].offset, { m });
						v.count++;
						found = true;

						// Update the offsets
						for (size_t j = 1; j < veg.size(); j++)
							veg[j].offset = veg[j - 1].offset + veg[j - 1].count;
					}
				}
				if (!found)
				{
					Vegetation v = {};
					v.heightOffset = newVeg.heightOffset;
					v.count = 1;
					v.density = 1;
					v.lodDist = 2000.0f;
					v.maxSlope = 0.0f;
					v.model = newVeg.model;
					v.minScale = 1.0f;
					v.maxScale = 1.0f;
					v.spacing = 1.0f;

					if (veg.size() > 0)
						v.offset = veg[veg.size() - 1].offset + veg[veg.size() - 1].count;
					else
						v.offset = 0;

					veg.push_back(v);
					vegData.push_back({ m });
				}

				return true;
			}
		}

		return false;
	}*/

	bool TerrainNode::LODSelect(float *lodVisRanges, int lodLevel, Camera *camera, std::vector<TerrainInstanceData> &data)
	{
		if (!InSphere(lodVisRanges[lodLevel], camera->GetPosition()))
			return false;

		if (!InFrustum(camera))
			return true;

		TerrainInstanceData d = {};

		if (lodLevel == 0)
		{
			d.params = glm::vec4(x, z, size, lodVisRanges[lodLevel]);
			data.push_back(d);
			return true;
		}
		else
		{
			if (!InSphere(lodVisRanges[lodLevel - 1], camera->GetPosition()))
			{
				d.params = glm::vec4(x, z, size, lodVisRanges[lodLevel]);
				data.push_back(d);
			}
			else
			{
				if (!topLeft->LODSelect(lodVisRanges, lodLevel - 1, camera, data))
				{
					d.params = glm::vec4(topLeft->x, topLeft->z, topLeft->size, lodVisRanges[topLeft->level]);
					data.push_back(d);
				}
				if (!topRight->LODSelect(lodVisRanges, lodLevel - 1, camera, data))
				{
					d.params = glm::vec4(topRight->x, topRight->z, topRight->size, lodVisRanges[topRight->level]);
					data.push_back(d);
				}
				if (!bottomLeft->LODSelect(lodVisRanges, lodLevel - 1, camera, data))
				{
					d.params = glm::vec4(bottomLeft->x, bottomLeft->z, bottomLeft->size, lodVisRanges[bottomLeft->level]);
					data.push_back(d);
				}
				if (!bottomRight->LODSelect(lodVisRanges, lodLevel - 1, camera, data))
				{
					d.params = glm::vec4(bottomRight->x, bottomRight->z, bottomRight->size, lodVisRanges[bottomRight->level]);
					data.push_back(d);
				}
			}

			return true;
		}
	}

	void TerrainNode::RecalculateNode(int x, int z, int size, int level, float *heights, int resolution)
	{
		this->x = (unsigned short)x;
		this->z = (unsigned short)z;
		this->size = (unsigned short)size;
		this->level = (unsigned short)level;

		if (level == 0)
		{
			topLeft = nullptr;
			topRight = nullptr;
			bottomLeft = nullptr;
			bottomRight = nullptr;

			// This is the last LOD
			// Find the min/max heights at this patch of terrain
			minHeight = 99999.0f;

			for (int j = z; j < z + size; j++)
			{
				for (int i = x; i < x + size; i++)
				{
					if (InBounds(i, j, resolution))
					{
						float newval = heights[j * resolution + i];

						if (newval < minHeight)
						{
							minHeight = newval;
						}
					}
				}
			}

			maxHeight = -99999.0f;
			for (int j = z; j < z + size; j++)
			{
				for (int i = x; i < x + size; i++)
				{
					if (InBounds(i, j, resolution))
					{
						float newval = heights[j * resolution + i];

						if (newval > maxHeight)
						{
							maxHeight = newval;
						}
					}
				}
			}
		}
		else
		{
			int halfSize = size / 2;

			topLeft->RecalculateNode(x, z, halfSize, level - 1, heights, resolution);
			topRight->RecalculateNode(x + halfSize, z, halfSize, level - 1, heights, resolution);
			bottomLeft->RecalculateNode(x, z + halfSize, halfSize, level - 1, heights, resolution);
			bottomRight->RecalculateNode(x + halfSize, z + halfSize, halfSize, level - 1, heights, resolution);

			maxHeight = std::max(std::max(topLeft->maxHeight, topRight->maxHeight), std::max(bottomLeft->maxHeight, bottomRight->maxHeight));
			minHeight = std::min(std::min(topLeft->minHeight, topRight->minHeight), std::min(bottomLeft->minHeight, bottomRight->minHeight));
		}
	}

	bool TerrainNode::InBounds(int x, int z, int resolution)
	{
		if (x < 0 || x >= resolution - 1 || z < 0 || z >= resolution - 1)
			return false;

		return true;
	}

	bool TerrainNode::InSphere(float radius, const glm::vec3 &position)
	{
		float rSqr = radius * radius;
		glm::vec3 min = glm::vec3(x, minHeight, z);
		glm::vec3 max = glm::vec3(x + size, maxHeight, z + size);
		glm::vec3 distV = glm::vec3(0.0f);

		if (position.x < min.x)
			distV.x = (position.x - min.x);
		else if (position.x > max.x)
			distV.x = (position.x - max.x);

		if (position.y < min.y)
			distV.y = (position.y - min.y);
		else if (position.y > max.y)
			distV.y = (position.y - max.y);

		if (position.z < min.z)
			distV.z = (position.z - min.z);
		else if (position.z > max.z)
			distV.z = (position.z - max.z);

		float distSqr = glm::dot(distV, distV);

		return distSqr <= rSqr;
	}

	bool TerrainNode::InFrustum(Camera *camera)
	{
		glm::vec3 min_v = glm::vec3(x, minHeight, z);
		glm::vec3 max_v = glm::vec3(x + size, maxHeight, z + size);

		bool b = false;

		if (camera->GetFrustum().BoxInFrustum(min_v, max_v) != FrustumIntersect::OUTSIDE)
		{
			b = true;
		}

		return b;
	}
}
