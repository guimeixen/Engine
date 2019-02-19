#include "Utils.h"

#include "Graphics\ResourcesLoader.h"
#include "Graphics\Material.h"
#include "Physics\RigidBody.h"

#include "include\glm\gtx\quaternion.hpp"

#include <fstream>
#include <iostream>
#include <sstream>

#include <Windows.h>

namespace Engine
{
	namespace utils
	{
		// From Graphics Gems 1, pg.548-550
		AABB RecomputeAABB(const AABB &aabb, const glm::mat4 &transform)
		{
			// Transform box, A, into another box, B, of the same form
			float minA[3] = { aabb.min.x, aabb.min.y, aabb.min.z };
			float maxA[3] = { aabb.max.x, aabb.max.y, aabb.max.z };

			float minB[3] = { 0.0f, 0.0f, 0.0f };
			float maxB[3] = { 0.0f, 0.0f, 0.0f };

			for (int i = 0; i < 3; i++)
			{
				for (int j = 0; j < 3; j++)
				{
					float x = transform[j][i] * minA[j];
					float y = transform[j][i] * maxA[j];
					minB[i] += glm::min(x, y);
					maxB[i] += glm::max(x, y);
				}
			}

			return AABB{ glm::vec3(minB[0], minB[1], minB[2]) + glm::vec3(transform[3]), glm::vec3(maxB[0], maxB[1], maxB[2]) + glm::vec3(transform[3]) };
		}

		float AngleFromPoint(const glm::vec2 &point)
		{
			float angle = glm::degrees(std::atan2(point.x, point.y));
			angle += 360.0f;
			return std::fmod(angle, 360.0f);
		}

		float WrapAngle(float startAngle, float goalAngle)
		{
			//float a = goalAngle - startAngle;
			//return std::fmodf(a + 180.0f, 360.0f) - 180.0f;

			return std::fmod(std::fmod(goalAngle - startAngle, 360.0f) + 540.0f, 360.0f) - 180.0f;
		}

		bool RayAABBIntersection(const glm::vec3 &o, const glm::vec3 &d, const AABB &aabb)
		{
			float tmin, tmax, tymin, tymax, tzmin, tzmax;
			int tNear = 0;
			int tFar = INT_MAX;

			float divx = 1 / d.x;
			float divy = 1 / d.y;
			float divz = 1 / d.z;

			if (divx >= 0)
			{
				tmin = (aabb.min.x - o.x) * divx;
				tmax = (aabb.max.x - o.x) * divx;
			}
			else
			{
				tmin = (aabb.max.x - o.x) * divx;
				tmax = (aabb.min.x - o.x) * divx;
			}

			if (divy >= 0)
			{
				tymin = (aabb.min.y - o.y) * divy;
				tymax = (aabb.max.y - o.y) * divy;
			}
			else
			{
				tymin = (aabb.max.y - o.y) * divy;
				tymax = (aabb.min.y - o.y) * divy;
			}
			if ((tmin > tymax) || (tymin > tmax))
			{
				return false;
			}
			if (tymin > tmin)
			{
				tmin = tymin;
			}
			if (tymax < tmax)
			{
				tmax = tymax;
			}

			if (divz >= 0)
			{
				tzmin = (aabb.min.z - o.z) * divz;
				tzmax = (aabb.max.z - o.z) * divz;
			}
			else
			{
				tzmin = (aabb.max.z - o.z) * divz;
				tzmax = (aabb.min.z - o.z) * divz;
			}
			if ((tmin > tzmax) || (tzmin > tmax))
			{
				return false;
			}
			if (tzmin > tmin)
			{
				tmin = tzmin;
			}
			if (tzmax < tmax)
			{
				tmax = tzmax;
			}

			return ((tmin < tFar) && (tmax > tNear));
		}

		bool AABBSphereIntersection(const AABB &aabb, const glm::vec3 &center, float r)
		{
			float r2 = r * r;
			float minDist = 0.0f;
			for (int i = 0; i < 3; i++)
			{
				if (center[i] < aabb.min[i])
					minDist += glm::length2(center[i] - aabb.min[i]);
				else if (center[i] > aabb.max[i])
					minDist += glm::length2(center[i] - aabb.max[i]);
			}
			return minDist <= r2;

			/*glm::vec3 closestPointInAabb = glm::min(glm::max(center, aabb.min), aabb.max);
			double distSqr = glm::length2((closestPointInAabb - center));

			// The AABB and the sphere overlap if the closest point within the rectangle is
			// within the sphere's radius
			return distSqr < (r * r);*/
		}

		glm::vec3 GetRayDirection(const glm::vec2 &point, Camera *camera)
		{
			glm::vec2 ndcCoords;
			ndcCoords.x = 2.0f * point.x / camera->GetWidth() - 1.0f;
			ndcCoords.y = 2.0f * point.y / camera->GetHeight() - 1.0f;

			// From ndc to clip space

			// Changed z to 0.0f because we're now using glClipControl with z [0,1] as in Vulkan

			glm::vec4 clipSpaceCoords = glm::vec4(ndcCoords.x, -ndcCoords.y, 0.0f, 1.0f);		// z = -1 means were are at the near plane. In OpenGL clip space is a cube with max 1,1,1 and min -1,-1,-1.
																								// In Vulkan is 1,1,1 and -1,-1,0. The same for Direct3D
																								// From clip space to view space
			glm::mat4 invProj = glm::inverse(camera->GetProjectionMatrix());
			glm::vec4 viewSpaceCoords = invProj * clipSpaceCoords;
			viewSpaceCoords.z = -1.0f;
			viewSpaceCoords.w = 0.0f;

			// From view space to world space
			glm::mat4 invView = glm::inverse(camera->GetViewMatrix());
			glm::vec4 worldSpaceCoords = invView * viewSpaceCoords;

			// Now that we have the world space coords, normalize it because we only want the direction
			return glm::normalize(glm::vec3(worldSpaceCoords));
		}

		bool CheckAABBPoint(const AABB &aabb, const glm::vec3 &point)
		{
			if (point.x >= aabb.min.x && point.x <= aabb.max.x)
			{
				if (point.y >= aabb.min.y && point.y <= aabb.max.y)
				{
					if (point.z >= aabb.min.z && point.z <= aabb.max.z)
					{
						return true;
					}
				}
			}

			return false;
		}

		bool AABBABBBIntersection(const AABB &a, const AABB &b)
		{
			// Two AABBs only intersect if they intersect on all 3 axes
			if (a.max.x < b.min.x || a.min.x > a.max.x) return false;
			if (a.max.y < b.min.y || a.min.y > a.max.y) return false;
			if (a.max.z < b.min.z || a.min.z > a.max.z) return false;

			return true;		// Intersecting on all 3 axes
		}

		void FindFilesInDirectory(std::vector<std::string> &files, const std::string &dir, const char *extension)
		{
			WIN32_FIND_DATAA findData;
			HANDLE h = FindFirstFileA(dir.c_str(), &findData);

			if (INVALID_HANDLE_VALUE == h)
				return;

			do
			{
				if (findData.cFileName[0] != '.' && findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
				{
					std::string path = dir;
					path.pop_back();		// Remove the * necessary to find the files
					path += findData.cFileName;
					path += "/*";
					FindFilesInDirectory(files, path.c_str(), extension);
				}
				else if (findData.cFileName[0] != '.' && (std::strstr(findData.cFileName, extension) > 0))
				{
					std::string path = dir;
					path.pop_back();		// Remove the * necessary to find the files
					files.push_back(path + std::string(findData.cFileName));
				}

			} while (FindNextFileA(h, &findData));

			FindClose(h);
		}
	}
}
