#pragma once

#include "Physics/BoundingVolumes.h"
#include "Graphics/Camera/Camera.h"

#include "Data/Shaders/common.glsl"

#include "include/glm/glm.hpp"

namespace Engine
{
	struct CSMInfo
	{
		float cascadeSplitEnd[CASCADE_COUNT];
		glm::mat4 viewProjLightSpace[CASCADE_COUNT];
		Camera cameras[CASCADE_COUNT];
		AABB csmAABB[CASCADE_COUNT];
	};

	namespace CascadedShadowMap
	{
		void Update(CSMInfo &csmInfo, Camera &camera, const glm::vec3 &lightDir);
	}
}
