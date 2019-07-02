#pragma once

#include "Frustum.h"

namespace Engine
{
	class Camera
	{
	public:
		Camera();
		virtual ~Camera();

		virtual void Update(float dt, bool doMovement);
		void UpdateFrustum(const glm::vec3 &position, const glm::vec3 &center, const glm::vec3 &up);

		void SetProjectionMatrix(float left, float right, float bottom, float top, float near, float far);
		void SetProjectionMatrix(float fov, int windowWidth, int windowHeight, float near, float far);
		void SetProjectionMatrix(const glm::mat4 &proj);
		void SetViewMatrix(const glm::mat4 &view);
		void SetViewMatrix(const glm::vec3 &pos, const glm::vec3 &center, const glm::vec3 &up);
		void SetNearPlane(float nearPlane);
		void SetFarPlane(float farPlane);
		void SetSensitivity(float sensitivity);
		void Resize(int windowWidth, int windowHeight);

		int GetWidth() const { return width; }
		int GetHeight() const { return height; }

		float GetNearPlane() const { return nearPlane; }
		float GetFarPlane() const { return farPlane; }
		float GetFov() const { return fov; }
		float GetYaw() const { return yaw; }				// Used script
		float GetPitch() const { return pitch; }			// Used script
		float GetAspectRatio() const { return aspectRatio; }
		float GetSensitivity() const { return sensitivity; }

		glm::vec3 &GetPosition() { return position; }		// Used script
		glm::vec3 &GetFront() { return front; }				// Used script
		glm::vec3 &GetRight() { return right; }				// Used script
		glm::vec3 &GetUp() { return up; }

		const glm::mat4 &GetViewMatrix() const { return viewMatrix; }
		const glm::mat4 &GetProjectionMatrix() const { return projectionMatrix; }

		const Frustum &GetFrustum() const { return frustum; }

		virtual void SetPosition(const glm::vec3 &position);
		virtual void SetYaw(float yaw);
		virtual void SetPitch(float pitch);
		void SetFrontAndUp(const glm::vec3 &front, const glm::vec3 &up);

		void Reset() { firstMove = true; }

		// Script functions
		glm::vec2 GetViewportSize() const { return glm::vec2(width, height); }
		glm::vec3 GetPositionScript() const { return position; }

	protected:
		Frustum frustum;

		bool firstMove;

		glm::vec3 position;
		glm::vec2 lastMousePos;
		glm::mat4 viewMatrix;
		glm::mat4 projectionMatrix;

		glm::vec3 front;
		glm::vec3 up;
		glm::vec3 right;

		float nearPlane;
		float farPlane;
		float aspectRatio;
		float fov;

		int width;
		int height;

		float yaw;
		float pitch;

		float sensitivity;
	};
}