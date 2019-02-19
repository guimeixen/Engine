#include "Camera.h"

#include "include\glm\gtc\matrix_transform.hpp"

namespace Engine
{
	Camera::Camera()
	{
		firstMove = true;
		sensitivity = 0.3f;
		position = glm::vec3();
		lastMousePos = glm::vec2();
		pitch = 0.0f;
		yaw = 0.0f;

		projectionMatrix = glm::mat4(1.0f);
		viewMatrix = glm::mat4(1.0f);
	}

	Camera::~Camera()
	{
	}

	void Camera::Update(float dt, bool doMovement)
	{
		frustum.Update(position, position + front, up);
	}

	void Camera::UpdateFrustum(const glm::vec3 &position, const glm::vec3 &center, const glm::vec3 &up)
	{
		frustum.Update(position, center, up);
	}

	void Camera::SetProjectionMatrix(float left, float right, float bottom, float top, float near, float far)
	{
		nearPlane = near;
		farPlane = far;

		projectionMatrix = glm::orthoRH(left, right, bottom, top, near, far);
		frustum.UpdateProjection(left, right, bottom, top, near, far);
	}

	void Camera::SetProjectionMatrix(float fov, int windowWidth, int windowHeight, float near, float far)
	{
		lastMousePos.x = windowWidth / 2.0f;
		lastMousePos.y = windowHeight / 2.0f;

		width = windowWidth;
		height = windowHeight;

		this->fov = fov;
		aspectRatio = (float)windowWidth / windowHeight;
		nearPlane = near;
		farPlane = far;

		projectionMatrix = glm::perspective(glm::radians(fov), aspectRatio, near, far);
		frustum.UpdateProjection(glm::radians(fov), aspectRatio, near, far);
	}

	void Camera::SetProjectionMatrix(const glm::mat4 &proj)
	{
		projectionMatrix = proj;
	}

	void Camera::SetViewMatrix(const glm::mat4 &view)
	{
		viewMatrix = view;
	}

	void Camera::SetNearPlane(float nearPlane)
	{
		this->nearPlane = nearPlane;
		projectionMatrix = glm::perspective(glm::radians(fov), aspectRatio, nearPlane, farPlane);
		frustum.UpdateProjection(glm::radians(fov), aspectRatio, nearPlane, farPlane);
	}

	void Camera::SetFarPlane(float farPlane)
	{
		this->farPlane = farPlane;
		projectionMatrix = glm::perspective(glm::radians(fov), aspectRatio, nearPlane, farPlane);
		frustum.UpdateProjection(glm::radians(fov), aspectRatio, nearPlane, farPlane);
	}

	void Camera::SetSensitivity(float sensitivity)
	{
		this->sensitivity = sensitivity;
	}

	void Camera::Resize(int windowWidth, int windowHeight)
	{
		this->width = windowWidth;
		this->height = windowHeight;
		this->aspectRatio = (float)windowWidth / windowHeight;
		projectionMatrix = glm::perspective(glm::radians(fov), aspectRatio, nearPlane, farPlane);
		frustum.UpdateProjection(glm::radians(fov), aspectRatio, nearPlane, farPlane);
	}

	void Camera::SetFrontAndUp(const glm::vec3 &front, const glm::vec3 &up)
	{
		this->front = front;
		this->up = up;
		right = glm::normalize(glm::cross(front, glm::vec3(0.0f, 1.0f, 0.0f)));
		//this->up = glm::normalize(glm::cross(right, front));

		viewMatrix = glm::lookAt(position, position + front, up);
		frustum.Update(position, position + front, up);
	}

	void Camera::SetPosition(const glm::vec3 &position)
	{
		this->position = position;
		viewMatrix = glm::lookAt(position, position + front, up);
		frustum.Update(position, position + front, up);
	}

	void Camera::SetYaw(float yaw)
	{
	}

	void Camera::SetPitch(float pitch)
	{
		this->pitch = pitch;

		front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
		front.y = sin(glm::radians(pitch));
		front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
		front = glm::normalize(front);

		right = glm::normalize(glm::cross(front, glm::vec3(0.0f, 1.0f, 0.0f)));  // Normalize the vectors, because their length gets closer to 0 the more you look up or down which results in slower movement.
		up = glm::normalize(glm::cross(right, front));

		viewMatrix = glm::lookAt(position, position + front, up);
		frustum.Update(position, position + front, up);
	}
}
