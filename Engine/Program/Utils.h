#pragma once

#include "Physics/BoundingVolumes.h"
#include "Graphics/Camera/Camera.h"

#include <string>
#include <vector>

namespace Engine
{
	namespace utils
	{
		AABB RecomputeAABB(const AABB &aabb, const glm::mat4 &transform);
		float AngleFromPoint(const glm::vec2 &point);
		float WrapAngle(float startAngle, float goalAngle);
		bool RayAABBIntersection(const glm::vec3 &o, const glm::vec3 &d, const AABB &aabb);
		bool AABBSphereIntersection(const AABB &aabb, const glm::vec3 &center, float r);		
		bool CheckAABBPoint(const AABB &aabb, const glm::vec3 &point);
		bool AABBABBBIntersection(const AABB &a, const AABB &b);
		glm::vec3 GetRayDirection(const glm::vec2& point, Camera* camera);
		glm::vec3 ProjectToUnitSphere(const glm::vec3& position);

		unsigned int Align(unsigned int value, unsigned int alignment);

		// It's necessary to clear the vector before calling the function if you don't want the values from a previous find to remain in the vector
		void FindFilesInDirectory(std::vector<std::string> &files, const std::string &dir, const char *extension, bool includeSubDirectories = true, bool addPathToFileName = true);
		bool CreateDir(const char *folderPath);
		bool DirectoryExists(const std::string &path);
		// Removes the extension from a file path but leaves the dot
		std::string RemoveExtensionFromFilePath(const std::string &path);
		void OpenFileWithDefaultProgram(const std::string &path);
	}
}
