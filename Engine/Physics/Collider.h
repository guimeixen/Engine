#pragma once

#include "Game/ComponentManagers/PhysicsManager.h"

class btCollisionShape;
class btCollisionObject;

namespace Engine
{
	class Collider
	{
	public:
		Collider();
		Collider(btCollisionObject *collider, btCollisionShape *shape);
		Collider(btCollisionShape *shape);

		btCollisionObject *GetHandle() const { return collider; }

		void GetTransform(glm::mat4 &transform) const;
		void SetTransform(const glm::mat4 &transform);
		void SetPosition(const glm::vec3 &pos);
		void SetCenter(const glm::vec3 &center);
		void SetEnabled(bool enable);

		void SetBoxSize(const glm::vec3 &size);
		void SetRadius(float radius);
		void SetCapsuleHeight(float height);

		glm::vec3 GetBoxSize() const;
		float GetRadius() const;
		float GetHeight() const;
		int GetShapeType() const;
		const glm::vec3 &GetCenter() const { return center; }

		void DebugView(bool enable) { debugView = enable; }
		bool WantsDebugView() const { return debugView; }

		void Serialize(Serializer &s);
		void Deserialize(PhysicsManager &physicsManager, Serializer &s);

		// Script functions
		glm::vec3 GetPosition();

	private:
		btCollisionObject *collider = nullptr;
		btCollisionShape *shape = nullptr;
		bool debugView = true;
		bool isEnabled = true;	
		glm::vec3 position;
		glm::vec3 center;
	};
}
