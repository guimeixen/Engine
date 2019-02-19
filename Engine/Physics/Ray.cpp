#include "Ray.h"

#include "Program\Input.h"
#include "Program\Log.h"

#include "include\glm\gtc\matrix_transform.hpp"

namespace Engine
{
	Ray::Ray()
	{
	}

	Ray::~Ray()
	{
	}

	void Ray::CalculateDirection(Camera *camera, const glm::vec2 &point)
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
		direction = glm::normalize(glm::vec3(worldSpaceCoords));
	}

	void Ray::SetOrigin(const glm::vec3 &origin)
	{
		this->origin = origin;
	}
}
