#include "Collider.h"

#include "include/bullet/btBulletDynamicsCommon.h"

#include "include/glm/gtc/type_ptr.hpp"

namespace Engine
{
	Collider::Collider()
	{
		center = glm::vec3(0.0f);
		position = glm::vec3(0.0f);
	}

	Collider::Collider(btCollisionObject *collider, btCollisionShape *shape)
	{
		this->collider = collider;
		this->shape = shape;
		center = glm::vec3(0.0f);
		position = glm::vec3(0.0f);
	}

	Collider::Collider(btCollisionShape *shape)
	{
		this->shape = shape;
		center = glm::vec3(0.0f);
		position = glm::vec3(0.0f);
	}

	void Collider::GetTransform(glm::mat4 &transform) const
	{
		btTransform t = collider->getWorldTransform();
		t.getOpenGLMatrix(glm::value_ptr(transform));
	}

	void Collider::SetTransform(const glm::mat4 &transform)
	{
		position = transform[3];

		glm::mat4 m = transform;

		// Scale is being removed in the object UpdateComponents function
		/*m[0] = glm::normalize(m[0]);		// Remove scale from the transform so we don't have problems when parenting objects and also the scale should be set separately using SetBoxSize/Radius, etc
		m[1] = glm::normalize(m[1]);
		m[2] = glm::normalize(m[2]);*/

		//glm::mat4 m2 = glm::mat4(1.0f);
		//m2[3] = glm::vec4(center.x, center.y, center.z, 1.0f);

		//m = m * m2;

		m[3] += glm::vec4(center, 1.0f);

		btTransform t;
		t.setFromOpenGLMatrix(glm::value_ptr(m));
		collider->setWorldTransform(t);
	}

	void Collider::SetPosition(const glm::vec3 &pos)
	{
		position = pos;
		btVector3 posBt = btVector3(pos.x + center.x, pos.y + center.y, pos.z + center.z);
		collider->getWorldTransform().setOrigin(posBt);
	}

	void Collider::SetCenter(const glm::vec3 &center)
	{
		this->center = center;
		btVector3 finalPos = btVector3(position.x + center.x, position.y + center.y, position.z + center.z);
		collider->getWorldTransform().setOrigin(finalPos);
	}

	void Collider::SetEnabled(bool enable)
	{
		isEnabled = enable;
		if (enable)
			collider->setCollisionFlags(collider->getCollisionFlags() & ~btCollisionObject::CollisionFlags::CF_NO_CONTACT_RESPONSE);
		else
			collider->setCollisionFlags(collider->getCollisionFlags() | btCollisionObject::CollisionFlags::CF_NO_CONTACT_RESPONSE);
	}

	void Collider::SetBoxSize(const glm::vec3 &size)
	{
		if (shape->getShapeType() == BOX_SHAPE_PROXYTYPE)
		{
			btBoxShape *box = static_cast<btBoxShape*>(shape);
			glm::vec3 halfExtents = size * 0.5f;
			halfExtents -= box->getMargin();

			/*btVector3 implicit = box->getHalfExtentsWithMargin();

			btVector3 scale;
			scale.setX(size.x / (implicit.x() * 2.0f));
			scale.setY(size.y / (implicit.y() * 2.0f));
			scale.setZ(size.z / (implicit.z() * 2.0f));

			box->setLocalScaling(scale);*/

			box->setImplicitShapeDimensions(btVector3(halfExtents.x, halfExtents.y, halfExtents.z));
		}
	}

	void Collider::SetRadius(float radius)
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

	void Collider::SetCapsuleHeight(float height)
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

	glm::vec3 Collider::GetBoxSize() const
	{
		if (shape->getShapeType() == BOX_SHAPE_PROXYTYPE)
		{
			btBoxShape *box = static_cast<btBoxShape*>(shape);
			btVector3 halfExtents = box->getHalfExtentsWithMargin();
			return glm::vec3(halfExtents.x() * 2.0f, halfExtents.y() * 2.0f, halfExtents.z() * 2.0f);
		}

		return glm::vec3(0.0f);
	}

	float Collider::GetRadius() const
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

	float Collider::GetHeight() const
	{
		if (shape->getShapeType() == CAPSULE_SHAPE_PROXYTYPE)
		{
			btCapsuleShape *capsule = static_cast<btCapsuleShape*>(shape);
			return capsule->getHalfHeight() * 2.0f + 2.0f * GetRadius();
			//return capsule->getHalfHeight() * 2.0f;
		}

		return 0.0f;
	}

	int Collider::GetShapeType() const
	{
		return shape->getShapeType();
	}

	void Collider::Serialize(Serializer &s)
	{
		int type = shape->getShapeType();
		s.Write(type);
		s.Write(isEnabled);
		s.Write(debugView);
		s.Write(collider->getFriction());
		s.Write(center);

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
			s.Write(capsule->getHalfHeight());
		}
	}

	void Collider::Deserialize(PhysicsManager &physicsManager, Serializer &s)
	{
		int type = 0;
		s.Read(type);
		s.Read(isEnabled);
		s.Read(debugView);
		float friction = 0.0f;
		s.Read(friction);
		s.Read(center);

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

		if (!collider)
		{
			collider = new btCollisionObject();
			collider->getWorldTransform().setOrigin(btVector3());
			collider->setCollisionShape(shape);
		}

		collider->setFriction(friction);

		SetEnabled(isEnabled);
	}

	glm::vec3 Collider::GetPosition()
	{
		btVector3 origin = collider->getWorldTransform().getOrigin();

		return glm::vec3(origin.x(), origin.y(), origin.z());
	}
}
