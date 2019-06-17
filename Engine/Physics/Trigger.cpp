#include "Trigger.h"

#include "Game/Script.h"

#include "include/glm/gtc/type_ptr.hpp"

#include <iostream>

namespace Engine
{
	Trigger::Trigger()
	{
		ghost = nullptr;
		shape = nullptr;
		script = nullptr;

		center = glm::vec3(0.0f);
		position = glm::vec3(0.0f);
	}

	Trigger::Trigger(PhysicsManager *physicsManager, btPairCachingGhostObject *trigger)
	{
		ghost = trigger;
		shape = ghost->getCollisionShape();
		script = nullptr;

		center = glm::vec3(0.0f);
		position = glm::vec3(0.0f);
	}

	void Trigger::Update(btDynamicsWorld *dynamicsWorld)
	{
		for (size_t i = 0; i < inTriggerList.size(); i++)
		{
			inTriggerList[i].id = std::numeric_limits<unsigned int>::max();
		}

		btManifoldArray manifoldArray;
		btBroadphasePairArray& pairArray = ghost->getOverlappingPairCache()->getOverlappingPairArray();
		int numPairs = pairArray.size();

		//std::cout << "pairs: " << numPairs << '\n';

		for (int i = 0; i < numPairs; ++i)
		{
			manifoldArray.clear();

			const btBroadphasePair& pair = pairArray[i];

			btBroadphasePair* collisionPair = dynamicsWorld->getPairCache()->findPair(pair.m_pProxy0, pair.m_pProxy1);

			if (!collisionPair) continue;

			if (collisionPair->m_algorithm)
				collisionPair->m_algorithm->getAllContactManifolds(manifoldArray);

			for (int j = 0; j < manifoldArray.size(); j++)
			{
				btPersistentManifold* manifold = manifoldArray[j];

				//std::cout << "Num contacts: " << manifold->getNumContacts() << '\n';

				if (manifold->getNumContacts() > 0)
				{
					if (inTriggerList.size() < (size_t)numPairs)
					{
						for (size_t i = inTriggerList.size(); i < (size_t)numPairs; i++)
							inTriggerList.push_back({std::numeric_limits<unsigned int>::max()});
					}

					Entity whoTriggered = { (unsigned int)ghost->getOverlappingObject(i)->getUserIndex() };

					inTriggerList[i] = whoTriggered;
				}
			}
		}

		if (oldInTriggerList.size() < inTriggerList.size())
			oldInTriggerList.resize(inTriggerList.size());
		
		for (size_t i = 0; i < inTriggerList.size(); i++)
		{
			if (inTriggerList[i].IsValid())
			{
				bool alreadyInList = false;
				size_t freeIndex = 0;
				for (size_t j = 0; j < oldInTriggerList.size(); j++)
				{
					if (inTriggerList[i].id == oldInTriggerList[j].id)
					{
						alreadyInList = true;
						//break;
					}
					if (oldInTriggerList[j].IsValid() == false)
						freeIndex = j;
				}

				// Call on trigger enter
				if (!alreadyInList)
				{
					if (script)
						script->CallOnTriggerEnter(inTriggerList[i]);
					
					//std::cout << "trigger enter\n";

					oldInTriggerList[freeIndex] = inTriggerList[i];
				}

				// OnTriggerStay
				if (script)
					script->CallOnTriggerStay(inTriggerList[i]);
			}
		}

		// OnTriggerExit
		for (size_t i = 0; i < oldInTriggerList.size(); i++)
		{
			bool stillOverlapping = false;
			for (size_t j = 0; j < inTriggerList.size(); j++)
			{
				if (oldInTriggerList[i].id == inTriggerList[j].id)
				{
					stillOverlapping = true;
					break;
				}
			}

			if (!stillOverlapping)
			{
				if (script)
					script->CallOnTriggerExit(oldInTriggerList[i]);

				//std::cout << "trigger exit\n";

				oldInTriggerList[i].id = std::numeric_limits<unsigned int>::max();
			}
		}
	}

	void Trigger::SetScript(Script *script)
	{
		this->script = script;
	}

	void Trigger::SetTransform(const glm::mat4 &transform)
	{
		position = transform[3];

		glm::mat4 m = glm::mat4(1.0f);
		m[3] = glm::vec4(center.x, center.y, center.z, 1.0f);

		m = transform * m;

		btTransform t;
		t.setFromOpenGLMatrix(glm::value_ptr(m));
		//t.setOrigin(btVector3(position.x + center.x, position.y + center.y, position.z + center.z));
		ghost->setWorldTransform(t);
	}

	void Trigger::SetRotation(const glm::quat &rot)
	{
		btQuaternion q(rot.x, rot.y, rot.z, rot.w);

		btTransform t1 = ghost->getWorldTransform();
		t1.setRotation(q);

		ghost->setWorldTransform(t1);
	}

	void Trigger::SetPosition(const glm::vec3 &pos)
	{
		position = pos;
		btVector3 posBt = btVector3(pos.x + center.x, pos.y + center.y, pos.z + center.z);

		ghost->getWorldTransform().setOrigin(posBt);
	}

	void Trigger::SetCenter(const glm::mat4 &transform, const glm::vec3 &center)
	{
		this->center = center;
		//btVector3 finalPos = btVector3(position.x + center.x, position.y + center.y, position.z + center.z);
		//ghost->getWorldTransform().setOrigin(finalPos);

		glm::mat4 m = glm::mat4(1.0f);
		m[3] = glm::vec4(center.x, center.y, center.z, 1.0f);
		m = transform * m;

		btTransform t;
		t.setFromOpenGLMatrix(glm::value_ptr(m));
		ghost->setWorldTransform(t);

		/*btTransform m = btTransform(btQuaternion(0.0f, 0.0f, 0.0f, 1.0f), btVector3(center.x, center.y, center.z));
		btTransform t = ghost->getWorldTransform();
		t.mult(m, t);	
		ghost->setWorldTransform(t);*/

		//btTransform t;
		//t.setFromOpenGLMatrix(glm::value_ptr(m));
		//t.setOrigin(btVector3(position.x + center.x, position.y + center.y, position.z + center.z));
		//ghost->setWorldTransform(t);	
	}

	void Trigger::SetBoxSize(const glm::vec3 &size)
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

	void Trigger::SetRadius(float radius)
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

	void Trigger::SetCapsuleHeight(float height)
	{
		if (height <= 0.0f)
			height = 0.1f;

		if (shape->getShapeType() == CAPSULE_SHAPE_PROXYTYPE)
		{
			//btVector3 scale = shape->getLocalScaling();
			//shape->setLocalScaling(btVector3(scale.x(), height, scale.z()));

			btCapsuleShape *capsule = static_cast<btCapsuleShape*>(shape);
			btVector3 dimensions = capsule->getImplicitShapeDimensions();
			dimensions.setY(height * 0.5f);
			capsule->setImplicitShapeDimensions(dimensions);
		}
	}

	void Trigger::EnableCollision()
	{
		ghost->setCollisionFlags(ghost->getCollisionFlags() & ~btCollisionObject::CollisionFlags::CF_NO_CONTACT_RESPONSE);
	}

	void Trigger::DisableCollision()
	{
		ghost->setCollisionFlags(ghost->getCollisionFlags() | btCollisionObject::CF_NO_CONTACT_RESPONSE);
	}

	bool Trigger::HasCollisionEnabled() const
	{
		return (ghost->getCollisionFlags() & btCollisionObject::CF_NO_CONTACT_RESPONSE) != btCollisionObject::CF_NO_CONTACT_RESPONSE;
	}

	void Trigger::GetTransform(glm::mat4 &transform) const
	{
		btTransform t = ghost->getWorldTransform();
		t.getOpenGLMatrix(glm::value_ptr(transform));
	}

	glm::vec3 Trigger::GetBoxSize() const
	{
		if (shape->getShapeType() == BOX_SHAPE_PROXYTYPE)
		{
			btBoxShape *box = static_cast<btBoxShape*>(shape);
			btVector3 halfExtents = box->getHalfExtentsWithMargin();
			return glm::vec3(halfExtents.x() * 2.0f, halfExtents.y() * 2.0f, halfExtents.z() * 2.0f);
		}

		return glm::vec3(0.0f);
	}

	float Trigger::GetRadius() const
	{
		int type = shape->getShapeType();

		if (type == SPHERE_SHAPE_PROXYTYPE)
		{
			btSphereShape *sphere = static_cast<btSphereShape*>(shape);
			return sphere->getRadius();
		}
		else if (type == CAPSULE_SHAPE_PROXYTYPE)
		{
			btCapsuleShape *capsule = static_cast<btCapsuleShape*>(shape);
			return capsule->getRadius();
		}

		return 0.0f;
	}

	float Trigger::GetHeight() const
	{
		if (shape->getShapeType() == CAPSULE_SHAPE_PROXYTYPE)
		{
			btCapsuleShape *capsule = static_cast<btCapsuleShape*>(shape);
			return capsule->getHalfHeight() * 2.0f + 2.0f * GetRadius();
			//return capsule->getHalfHeight() * 2.0f;
		}

		return 0.0f;
	}

	int Trigger::GetShapeType() const
	{
		return shape->getShapeType();
	}

	glm::vec3 Trigger::GetPosition() const
	{
		btVector3 origin = ghost->getWorldTransform().getOrigin();

		return glm::vec3(origin.x(), origin.y(), origin.z());
	}

	void Trigger::Serialize(Serializer &s)
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
			s.Write(shape->getLocalScaling().x());
		}
		else if (type == CAPSULE_SHAPE_PROXYTYPE)
		{
			btCapsuleShape *capsule = static_cast<btCapsuleShape*>(shape);
			s.Write(capsule->getLocalScaling().x());
			s.Write(capsule->getHalfHeight());
		}		

		s.Write(center);

		if (HasCollisionEnabled())
			s.Write(true);
		else
			s.Write(false);
	}

	void Trigger::Deserialize(PhysicsManager &physicsManager, Serializer &s)
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
			float halfHeight;
			s.Read(radius);
			s.Read(halfHeight);
			if (!shape)
				shape = physicsManager.GetCapsuleShape(radius, halfHeight * 2.0f);
			else
			{
				SetRadius(radius);
				SetCapsuleHeight(halfHeight * 2.0f);
			}
		}

		s.Read(center);

		bool hasCollisionEnabled = false;
		s.Read(hasCollisionEnabled);
		
		if (!ghost)
		{
			btTransform t(btQuaternion(0.0f, 0.0f, 0.0f, 1.0f), btVector3(0.0f, 0.0f, 0.0f));

			ghost = new btPairCachingGhostObject();
			ghost->setCollisionShape(shape);
			ghost->setWorldTransform(t);
			ghost->setCollisionFlags(ghost->getCollisionFlags() | btCollisionObject::CF_NO_CONTACT_RESPONSE);
		}

		if (ghost)
		{
			//SetCenter(center);

			if (hasCollisionEnabled)
				EnableCollision();
			else
				DisableCollision();
		}
	}
}
