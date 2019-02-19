#pragma once

#include "Game\ComponentManagers\PhysicsManager.h"
#include "Program\Serializer.h"

namespace Engine
{
	class RigidBody
	{
	public:
		RigidBody();
		RigidBody(btCollisionShape *shape, float mass);
		~RigidBody();

		btRigidBody *GetHandle() const { return rigidBody; }

		void GetTransform(glm::mat4 &transform) const;
		void SetTransform(const glm::mat4 &transform);

		void Activate();
		void SetPosition(const glm::vec3 &pos);
		void SetCenter(const glm::vec3 &c);
		void SetRotation(const glm::quat &rot);
		void SetMass(float mass);
		void SetDamping(float linearDamping, float angularDamping);
		void SetRestitution(float restitution);
		void SetAngularFactor(const glm::vec3 &angFactor);

		void ApplyForce(const glm::vec3 &dir);

		float GetMass() const { return mass; }
		float GetRestitution() const { return rigidBody->getRestitution(); }
		glm::vec3 GetAngularFactor() const;
		glm::vec3 GetLinearFactor() const;
		bool IsKinematic() const;

		void SetBoxSize(const glm::vec3 &size);
		void SetRadius(float radius);
		void SetCapsuleHeight(float height);

		const glm::vec3 &GetCenter() const { return center; }

		glm::vec3 GetBoxSize() const;
		float GetRadius() const;
		float GetHeight() const;
		int GetShapeType() const;

		void DebugView(bool enable) { debugView = enable; }
		bool WantsDebugView() const { return debugView; }

		void Serialize(Serializer &s);
		void Deserialize(PhysicsManager &physicsManager, Serializer &s);

		// Script functions
		void SetKinematic(bool isKinematic);
		void SetLinearVelocity(float x, float y, float z);
		void SetLinearFactor(float x, float y, float z);
		void ApplyCentralImpulse(float x, float y, float z);
		void DisableDeactivation();
		glm::vec3 GetPosition();
		glm::vec3 GetLinearVelocity();

	private:
		btDefaultMotionState *motionState;
		btRigidBody *rigidBody;
		btCollisionShape *shape;
		float mass;
		btVector3 inertia;
		bool debugView = true;
		glm::vec3 position;
		glm::vec3 center;
	};
}
