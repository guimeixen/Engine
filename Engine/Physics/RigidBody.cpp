#include "RigidBody.h"

#include "Program\Log.h"

#include "include\glm\gtc\type_ptr.hpp"

#include <iostream>

namespace Engine
{
	RigidBody::RigidBody()
	{
		motionState = nullptr;
		rigidBody = nullptr;
		shape = nullptr;

		center = glm::vec3(0.0f);
	}

	RigidBody::RigidBody(btCollisionShape *shape, float mass)
	{
		center = glm::vec3(0.0f);

		motionState = new btDefaultMotionState(btTransform(btQuaternion(0.0f, 0.0f, 0.0f, 1.0f), btVector3(0.0f, 0.0f, 0.0f)));

		this->mass = mass;
		inertia = btVector3(0.0f, 0.0f, 0.0f);
		shape->calculateLocalInertia(mass, inertia);
		this->shape = shape;

		btRigidBody::btRigidBodyConstructionInfo rigidBodyCI(mass, motionState, shape, inertia);

		rigidBody = new btRigidBody(rigidBodyCI);

		if (!rigidBody)
		{
			Log::Print(LogLevel::LEVEL_ERROR, "Failed to create rigid body\n");
			return;
		}

		if (mass < 0.01f)
			rigidBody->setCollisionFlags(rigidBody->getCollisionFlags() | btCollisionObject::CF_STATIC_OBJECT);
	}

	RigidBody::~RigidBody()
	{
	}

	void RigidBody::SetTransform(const glm::mat4 &transform)
	{
		glm::mat4 m = transform;
		position = transform[3];
		m[3] += glm::vec4(center, 0.0f);

		btTransform t;
		t.setFromOpenGLMatrix(glm::value_ptr(m));
		motionState->setWorldTransform(t);
		rigidBody->setWorldTransform(t);

		/*position = transform[3];

		glm::mat4 m = glm::mat4(1.0f);
		m[3] = glm::vec4(center, 1.0f);

		m = transform * m;

		btTransform t;
		t.setFromOpenGLMatrix(glm::value_ptr(m));
		motionState->setWorldTransform(t);
		rigidBody->setWorldTransform(t);*/
	}

	void RigidBody::GetTransform(glm::mat4 &transform) const
	{
		btTransform t = rigidBody->getWorldTransform();
		t.getOpenGLMatrix(glm::value_ptr(transform));
	}

	void RigidBody::Activate()
	{
		rigidBody->activate();
	}

	void RigidBody::SetPosition(const glm::vec3 &pos)
	{
		position = pos;

		btTransform t1;
		motionState->getWorldTransform(t1);
		btTransform t2 = rigidBody->getWorldTransform();

		t1.setOrigin(btVector3(pos.x + center.x, pos.y + center.y, pos.z + center.z));
		t2.setOrigin(btVector3(pos.x + center.x, pos.y + center.y, pos.z + center.z));

		motionState->setWorldTransform(t1);
		rigidBody->setWorldTransform(t2);
	}

	void RigidBody::SetCenter(const glm::vec3 &c)
	{
		btTransform t1;
		motionState->getWorldTransform(t1);
		btTransform t2 = rigidBody->getWorldTransform();

		center = c;
		btVector3 finalPos = btVector3(position.x + center.x, position.y + center.y, position.z + center.z);

		t1.setOrigin(finalPos);
		t2.setOrigin(finalPos);

		motionState->setWorldTransform(t1);
		rigidBody->setWorldTransform(t2);
	}

	void RigidBody::SetRotation(const glm::quat &rot)
	{
		btQuaternion q(rot.x, rot.y, rot.z, rot.w);

		btTransform t1;
		motionState->getWorldTransform(t1);
		btTransform t2 = rigidBody->getWorldTransform();

		t1.setRotation(q);
		t2.setRotation(q);

		motionState->setWorldTransform(t1);
		rigidBody->setWorldTransform(t2);
	}

	// This doesn't work. You need to remove the rigid body from the world, change the mass and then add it again. For now I'm setting mass initially in the constructor
	void RigidBody::SetMass(float mass)
	{
		this->mass = mass;
		shape->calculateLocalInertia(mass, inertia);
		rigidBody->setMassProps(mass, inertia);
	}

	void RigidBody::SetDamping(float linearDamping, float angularDamping)
	{
		rigidBody->setDamping(linearDamping, angularDamping);
	}

	void RigidBody::SetRestitution(float restitution)
	{
		rigidBody->setRestitution(restitution);
	}

	void RigidBody::SetKinematic(bool isKinematic)
	{
		if (isKinematic)
			rigidBody->setCollisionFlags(rigidBody->getCollisionFlags() | btCollisionObject::CollisionFlags::CF_KINEMATIC_OBJECT);
		else
		{
			rigidBody->setCollisionFlags(rigidBody->getCollisionFlags() & ~btCollisionObject::CollisionFlags::CF_KINEMATIC_OBJECT);
			rigidBody->activate();
		}
	}

	glm::vec3 RigidBody::GetAngularFactor() const
	{
		btVector3 angFactor = rigidBody->getAngularFactor();
		return glm::vec3(angFactor.x(), angFactor.y(), angFactor.z());
	}

	glm::vec3 RigidBody::GetLinearFactor() const
	{
		btVector3 linFactor = rigidBody->getLinearFactor();
		return glm::vec3(linFactor.x(), linFactor.y(), linFactor.z());
	}

	bool RigidBody::IsKinematic() const
	{
		if (rigidBody->getCollisionFlags() & btCollisionObject::CollisionFlags::CF_KINEMATIC_OBJECT)
			return true;
		else
			return false;
	}

	void RigidBody::SetBoxSize(const glm::vec3 &size)
	{
		if (shape->getShapeType() == BOX_SHAPE_PROXYTYPE)
		{
			//btVector3 scale = btVector3(size.x, size.y, size.z);
			//shape->setLocalScaling(scale);

			btBoxShape *box = static_cast<btBoxShape*>(shape);
			glm::vec3 halfExtents = size * 0.5f;
			halfExtents -= box->getMargin();
			box->setImplicitShapeDimensions(btVector3(halfExtents.x, halfExtents.y, halfExtents.z));
		}
	}

	void RigidBody::SetRadius(float radius)
	{
		if (radius <= 0.0f)
			radius = 0.1f;

		if (shape->getShapeType() == SPHERE_SHAPE_PROXYTYPE)
		{
			btSphereShape *sphere = static_cast<btSphereShape*>(shape);
			btVector3 dimensions = btVector3(radius, radius, radius);
			sphere->setImplicitShapeDimensions(dimensions);

			//btVector3 scale = btVector3(radius, radius, radius);
			//shape->setLocalScaling(scale);
		}
		else if (shape->getShapeType() == CAPSULE_SHAPE_PROXYTYPE)
		{
			btCapsuleShape *capsule = static_cast<btCapsuleShape*>(shape);
			//btVector3 scale = btVector3(radius, capsule->getHalfHeight() * 2.0f, radius);
			//capsule->setLocalScaling(scale);

			btVector3 dimensions = capsule->getImplicitShapeDimensions();
			dimensions.setX(radius);
			capsule->setImplicitShapeDimensions(dimensions);
		}
	}

	void RigidBody::SetCapsuleHeight(float height)
	{
		if (height <= 0.0f)
			height = 0.1f;

		if (shape->getShapeType() == CAPSULE_SHAPE_PROXYTYPE)
		{
			//btVector3 scale = shape->getLocalScaling();
			//shape->setLocalScaling(btVector3(scale.x(), height, scale.z()));

			btCapsuleShape *capsule = static_cast<btCapsuleShape*>(shape);
			btVector3 dimensions = capsule->getImplicitShapeDimensions();

			float cylinderHeight = height - 2.0f * GetRadius();

			dimensions.setY(cylinderHeight * 0.5f);
			capsule->setImplicitShapeDimensions(dimensions);
		}
	}

	glm::vec3 RigidBody::GetBoxSize() const
	{
		if (shape->getShapeType() == BOX_SHAPE_PROXYTYPE)
		{
			btBoxShape *box = static_cast<btBoxShape*>(shape);
			btVector3 halfExtents = box->getHalfExtentsWithMargin();
			return glm::vec3(halfExtents.x() * 2.0f, halfExtents.y() * 2.0f, halfExtents.z() * 2.0f);
		}

		return glm::vec3(0.0f);
	}

	float RigidBody::GetRadius() const
	{
		int type = shape->getShapeType();

		if (shape->getShapeType() == SPHERE_SHAPE_PROXYTYPE)
		{
			btSphereShape *sphere = static_cast<btSphereShape*>(shape);
			return sphere->getRadius();
		}
		else if (shape->getShapeType() == CAPSULE_SHAPE_PROXYTYPE)
		{
			btCapsuleShape *capsule = static_cast<btCapsuleShape*>(shape);
			return capsule->getRadius();
		}

		return 0.0f;
	}

	float RigidBody::GetHeight() const
	{
		if (shape->getShapeType() == CAPSULE_SHAPE_PROXYTYPE)
		{
			btCapsuleShape *capsule = static_cast<btCapsuleShape*>(shape);
			return capsule->getHalfHeight() * 2.0f + 2.0f * GetRadius();
			//return capsule->getHalfHeight() * 2.0f;
		}

		return 0.0f;
	}

	int RigidBody::GetShapeType() const
	{
		return shape->getShapeType();
	}

	void RigidBody::SetAngularFactor(const glm::vec3 &angFactor)
	{
		rigidBody->setAngularFactor(btVector3(angFactor.x, angFactor.y, angFactor.z));
	}

	void RigidBody::ApplyForce(const glm::vec3 &dir)
	{
		rigidBody->activate();
		rigidBody->applyCentralForce(btVector3(dir.x, dir.y, dir.z));
	}

	glm::vec3 RigidBody::GetLinearVelocity()
	{
		btVector3 vel = rigidBody->getLinearVelocity();

		return glm::vec3(vel.x(), vel.y(), vel.z());
	}

	void RigidBody::SetLinearVelocity(float x, float y, float z)
	{
		rigidBody->setLinearVelocity(btVector3(x, y, z));
	}

	void RigidBody::ApplyCentralImpulse(float x, float y, float z)
	{
		rigidBody->applyCentralImpulse(btVector3(x, y, z));
	}

	void RigidBody::DisableDeactivation()
	{
		rigidBody->forceActivationState(DISABLE_DEACTIVATION);
	}

	glm::vec3 RigidBody::GetPosition()
	{
		btTransform t;
		rigidBody->getMotionState()->getWorldTransform(t);
		btVector3 origin = t.getOrigin();

		return glm::vec3(origin.x(), origin.y(), origin.z());
	}

	void RigidBody::SetLinearFactor(float x, float y, float z)
	{
		rigidBody->setLinearFactor(btVector3(x, y, z));
	}

	void RigidBody::Serialize(Serializer &s)
	{
		int type = shape->getShapeType();
		s.Write(type);

		if (type == BOX_SHAPE_PROXYTYPE)
		{
			btBoxShape *box = static_cast<btBoxShape*>(shape);
			btVector3 halfExtents = box->getHalfExtentsWithMargin();
			s.Write(glm::vec3(halfExtents.x(), halfExtents.y(), halfExtents.z()));
		}
		else if (type == SPHERE_SHAPE_PROXYTYPE)
		{
			btSphereShape *sphere = static_cast<btSphereShape*>(shape);
			s.Write(sphere->getRadius());
		}
		else if (type == CAPSULE_SHAPE_PROXYTYPE)
		{
			btCapsuleShape *capsule = static_cast<btCapsuleShape*>(shape);
			s.Write(capsule->getRadius());
			s.Write(capsule->getHalfHeight() * 2.0f + 2.0f * capsule->getRadius());
		}

		s.Write(mass);
		s.Write(IsKinematic());
		s.Write(GetRestitution());
		s.Write(GetAngularFactor());
		s.Write(rigidBody->getAngularSleepingThreshold());
		s.Write(rigidBody->getLinearSleepingThreshold());
		s.Write(center);
	}

	void RigidBody::Deserialize(PhysicsManager &physicsManager, Serializer &s)
	{
		int type = 0;
		s.Read(type);

		if (type == BOX_SHAPE_PROXYTYPE)
		{
			glm::vec3 halfExtents;
			s.Read(halfExtents);
			if (!shape)
				shape = physicsManager.GetBoxShape(btVector3(halfExtents.x, halfExtents.y, halfExtents.z));
			else
				SetBoxSize(halfExtents * 2.0f);
		}
		else if (type == SPHERE_SHAPE_PROXYTYPE)
		{
			float radius;
			s.Read(radius);
			if (!shape)
				shape = physicsManager.GetSphereShape(radius);
			else
				SetRadius(radius);
		}
		else if (type == CAPSULE_SHAPE_PROXYTYPE)
		{
			float radius;
			float height;
			s.Read(radius);
			s.Read(height);
			if (!shape)
				shape = physicsManager.GetCapsuleShape(radius, height);
			else
			{
				SetRadius(radius);
				SetCapsuleHeight(height);
			}
		}

		bool isKinematic;
		float restitution;
		glm::vec3 angFactor;
		float angSleepThres;
		float linSleepThres;

		s.Read(mass);
		s.Read(isKinematic);
		s.Read(restitution);
		s.Read(angFactor);
		s.Read(angSleepThres);
		s.Read(linSleepThres);
		s.Read(center);

		if (!rigidBody)
		{
			motionState = new btDefaultMotionState(btTransform(btQuaternion(0.0f, 0.0f, 0.0f, 1.0f), btVector3(0.0f, 0.0f, 0.0f)));
			inertia = btVector3(0.0f, 0.0f, 0.0f);
			shape->calculateLocalInertia(mass, inertia);

			btRigidBody::btRigidBodyConstructionInfo rigidBodyCI(mass, motionState, shape, inertia);
			rigidBody = new btRigidBody(rigidBodyCI);

			//if (!rigidBody)
			//	Log::Message("Error! Failed to create rigid body");

			if (mass < 0.01f)
				rigidBody->setCollisionFlags(rigidBody->getCollisionFlags() | btCollisionObject::CF_STATIC_OBJECT);
		}

		if (rigidBody)
		{
			SetKinematic(isKinematic);
			SetRestitution(restitution);
			SetAngularFactor(angFactor);
			rigidBody->setSleepingThresholds(linSleepThres, angSleepThres);
		}
	}
}
