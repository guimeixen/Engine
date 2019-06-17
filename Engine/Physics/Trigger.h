#pragma once

#include "Game/ComponentManagers/PhysicsManager.h"

#include <vector>

class btCollisionShape;

namespace Engine
{
	class Script;
	class PhysicsManager;
	class RuntimeScript;

	class Trigger
	{
	public:
		Trigger();
		Trigger(PhysicsManager *physicsManager, btPairCachingGhostObject *trigger);

		//btGhostObject *GetHandle() const { return ghost; }
		btPairCachingGhostObject *GetHandle() const { return ghost; }

		void Update(btDynamicsWorld *dynamicsWorld);

		void SetScript(Script *script);
		void SetTransform(const glm::mat4 &transform);
		void SetRotation(const glm::quat &rot);
		void SetPosition(const glm::vec3 &pos);
		void SetCenter(const glm::mat4 &transform, const glm::vec3 &center);
		void SetBoxSize(const glm::vec3 &size);
		void SetRadius(float radius);
		void SetCapsuleHeight(float height);

		void EnableCollision();
		void DisableCollision();
		bool HasCollisionEnabled() const;

		void GetTransform(glm::mat4 &transform) const;
		glm::vec3 GetBoxSize() const;
		float GetRadius() const;
		float GetHeight() const;
		int GetShapeType() const;
		glm::vec3 GetPosition() const;
		const glm::vec3 &GetCenter() const { return center; }

		Script *GetScript() const { return script; }

		void DebugView(bool enable) { debugView = enable; }
		bool WantsDebugView() const { return debugView; }

		void Serialize(Serializer &s);
		void Deserialize(PhysicsManager &physicsManager, Serializer &s);

	private:
		btPairCachingGhostObject *ghost;
		btCollisionShape *shape;
		Script *script;
		std::vector<Entity> inTriggerList;
		std::vector<Entity> oldInTriggerList;
		glm::vec3 position;
		glm::vec3 center;
		bool debugView = true;
	};
}
