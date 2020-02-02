#include "FPSCamera.h"

#include "Program/Input.h"
#include "Program/Log.h"

#include "include/glm/gtc/matrix_transform.hpp"
#include "include/glm/gtx/euler_angles.hpp"

namespace Engine
{
	FPSCamera::FPSCamera()
	{
		worldUp = glm::vec3(0.0f, 1.0f, 0.0f);
		front = glm::vec3(0.0f, 0.0f, -1.0f);
		up = worldUp;
		right = glm::vec3(1.0f, 0.0f, 0.0f);
		moveSpeed = 8.0f;

		UpdateCameraVectors();
	}

	FPSCamera::~FPSCamera()
	{
	}

	void FPSCamera::Update(float deltaTime, bool doMovement, bool needRightMouse)
	{
		// Movement
		if (needRightMouse)
		{
			if (Input::IsMouseButtonDown(MouseButtonType::Right))
			{
				if (doMovement)
				{
					DoMovement(deltaTime);
				}

				DoLook();
			}
			else
			{
				Reset();
			}
		}
		else
		{
			if (doMovement)
			{
				DoMovement(deltaTime);
			}

			DoLook();
		}
	}

	void FPSCamera::SetPosition(const glm::vec3 &pos)
	{
		position = pos;
		UpdateCameraVectors();
	}

	void FPSCamera::SetPitch(float pitch)
	{
		this->pitch = pitch;
		UpdateCameraVectors();
	}

	void FPSCamera::SetYaw(float yaw)
	{
		this->yaw = yaw;
		UpdateCameraVectors();
	}

	void FPSCamera::DoMovement(float dt)
	{
		float velocity = moveSpeed * dt;

		if (Input::IsKeyPressed(KEY_W))		// w
		{
			position += front * velocity;
		}
		if (Input::IsKeyPressed(KEY_S))		// s
		{
			position -= front * velocity;
		}
		if (Input::IsKeyPressed(KEY_A))		// a
		{
			position -= right * velocity;
		}
		if (Input::IsKeyPressed(KEY_D))		// d
		{
			position += right * velocity;
		}
		if (Input::IsKeyPressed(KEY_E))		// e
		{
			position += worldUp * velocity;
		}
		if (Input::IsKeyPressed(KEY_Q))		// q
		{
			position -= worldUp * velocity;
		}
	}

	void FPSCamera::DoLook()
	{
		glm::vec2 mousePos = Input::GetMousePosition();

		if (firstMove == true)
		{
			lastMousePos = mousePos;
			firstMove = false;
		}

		float offsetX = lastMousePos.x - mousePos.x;
		float offsetY = mousePos.y - lastMousePos.y;
		lastMousePos = mousePos;

		offsetX *= sensitivity;
		offsetY *= sensitivity;

		yaw += offsetX;
		pitch += offsetY;

		if (pitch > 89.0f)
			pitch = 89.0f;
		if (pitch < -89.0f)
			pitch = -89.0f;

		UpdateCameraVectors();
	}

	void FPSCamera::UpdateCameraVectors()
	{
		glm::mat4 m = glm::eulerAngleYX(glm::radians(yaw), glm::radians(pitch));

		front = glm::normalize(m[2]);
		right = glm::normalize(glm::cross(front, glm::vec3(0.0f, 1.0f, 0.0f)));
		up = glm::normalize(glm::cross(right, front));

		viewMatrix = glm::lookAt(position, position + front, up);
		frustum.Update(position, position + front, up);
	}
}
