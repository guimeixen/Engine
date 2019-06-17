#pragma once

#include "Camera.h"

namespace Engine
{
	class FPSCamera : public Camera
	{
	public:
		FPSCamera();
		~FPSCamera();

		void Update(float deltaTime, bool doMovement, bool needRightMouse);

		void SetPosition(const glm::vec3 &pos);
		void SetPitch(float pitch);
		void SetYaw(float yaw);
		void SetMoveSpeed(float speed) { moveSpeed = speed; }

		float GetMoveSpeed() const { return moveSpeed; }

	private:
		void DoMovement(float dt);
		void DoLook();
		void UpdateCameraVectors();

	private:
		glm::vec3 worldUp;

		float moveSpeed;
	};
}
