#pragma once

#include "include\glm\glm.hpp"

namespace Engine
{
	// Use center, half width representation to use 16 bytes instead of 24 bytes
	struct AABB
	{
		glm::vec3 min;
		glm::vec3 max;
	};

	struct OBB
	{
		glm::vec3 center;
		glm::vec3 basis[3];
		glm::vec3 halfExt;
	};
}
