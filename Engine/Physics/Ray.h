#pragma once

#include "Graphics\Camera\Camera.h"

#include "include\glm\glm.hpp"

namespace Engine
{
	class Ray
	{
	public:
		Ray();
		~Ray();

		void CalculateDirection(Camera *camera, const glm::vec2 &point);

		const glm::vec3 &GetRayDirection() const { return direction; }
		const glm::vec3 &GetOrigin() const { return origin; }

		void SetOrigin(const glm::vec3 &origin);
		void SetDirection(const glm::vec3 &dir) { direction = dir; }

	private:
		glm::vec3 origin;
		glm::vec3 direction;
	};
}
