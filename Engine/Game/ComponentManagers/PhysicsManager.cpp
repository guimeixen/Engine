#include "PhysicsManager.h"

#include "Program\Log.h"
#include "Physics\RigidBody.h"
#include "ScriptManager.h"
#include "Physics\Collider.h"
#include "Physics\Trigger.h"
#include "Graphics\Effects\DebugDrawManager.h"
#include "TransformManager.h"

#include "Program\Utils.h"

#include "include\glm\gtc\matrix_transform.hpp"

#include <iostream>

namespace Engine
{
	PhysicsManager::PhysicsManager()
	{
		terrainCollider = nullptr;
		isInit = false;
	}

	PhysicsManager::~PhysicsManager()
	{
	}

	void PhysicsManager::Init(TransformManager *transformManager)
	{
		if (isInit)
			return;

		this->transformManager = transformManager;

		broadphase = new btDbvtBroadphase();

		collisionConfiguration = new btDefaultCollisionConfiguration();
		dispatcher = new btCollisionDispatcher(collisionConfiguration);

		solver = new btSequentialImpulseConstraintSolver;

		ghostCallback = new btGhostPairCallback();

		// The world
		dynamicsWorld = new btDiscreteDynamicsWorld(dispatcher, broadphase, solver, collisionConfiguration);
		dynamicsWorld->setGravity(btVector3(0.0f, -9.81f, 0.0f));
		dynamicsWorld->getPairCache()->setInternalGhostPairCallback(ghostCallback);			// For the ghost to work correctly

		isInit = true;
	}

	void PhysicsManager::Play()
	{
		for (size_t i = 0; i < usedRigidBodies; i++)
		{
			rigidBodies[i].rb->GetHandle()->activate();
		}
	}

	void PhysicsManager::Simulate(float dt)
	{
		dynamicsWorld->stepSimulation(dt, 5);

		for (size_t i = 0; i < usedRigidBodies; i++)
		{
			const RigidBodyInstance &rbi = rigidBodies[i];
			RigidBody *rb = rbi.rb;
			if (rb->GetHandle()->isActive() && !rb->IsKinematic())
			{
				glm::mat4 localToWorld = transformManager->GetLocalToWorld(rbi.e);
				//glm::vec3 worldPos = localToWorld[3];

				// If this object participates as an obstacle for the pathfinding grid then update the position
				//if (layer == Layer::OBSTACLE)
				//	game->GetAISystem().GetAStarGrid().UpdateNode(glm::vec2(worldPos.x, worldPos.z), true);

				rb->GetTransform(localToWorld);
				localToWorld[3] -= glm::vec4(rb->GetCenter(), 0.0f);
				transformManager->SetLocalToWorld(rbi.e, localToWorld);

				//worldPos = localToWorld[3];

				//if (layer == Layer::OBSTACLE)
				//	game->GetAISystem().GetAStarGrid().UpdateNode(glm::vec2(worldPos.x, worldPos.z), false);
			}
		}

		for (size_t i = 0; i < usedTriggers; i++)
		{
			const TriggerInstance &ti = triggers[i];
			if(ti.tr->GetScript())							// Only check for trigger overlaps if the entity has a script to call the onTriggerEnter, etc functions
				ti.tr->Update(dynamicsWorld);
		}
	}

	void PhysicsManager::Update()
	{
		/*const ModifiedTransform *mts = transformManager->GetModifiedTransforms();
		for (size_t i = 0; i < transformManager->GetNumModifiedTransforms(); i++)
		{
			glm::mat4 localToWorld = *mts[i].localToWorld;
			localToWorld[0] = glm::normalize(localToWorld[0]);
			localToWorld[1] = glm::normalize(localToWorld[1]);
			localToWorld[2] = glm::normalize(localToWorld[2]);

			TriggerInstance ti = {};
			if (GetTrigger(mts[i].e, ti))
			{
				ti.tr->SetTransform(localToWorld);
			}

			ColliderInstance ci = {};
			if (GetCollider(mts[i].e, ci))
			{
				ci.col->SetTransform(localToWorld);
			}

			RigidBodyInstance rbi = {};
			if (GetRigidBody(mts[i].e, rbi))
			{
				rbi.rb->SetPosition(localToWorld[3]);
			}
		}*/
	}	

	void PhysicsManager::PrepareDebugDraw(DebugDrawManager *debugDrawMngr)
	{
		for (size_t i = 0; i < usedTriggers; i++)
		{
			Trigger *t = triggers[i].tr;
			if (t->WantsDebugView())
			{
				if (t->GetShapeType() == BOX_SHAPE_PROXYTYPE)
				{
					glm::mat4 m;
					t->GetTransform(m);
					m = glm::scale(m, t->GetBoxSize());
					debugDrawMngr->AddCube(m, glm::vec3(1.0f, 1.0f, 0.0f));		// Render triggers as yellow
				}
				else if (t->GetShapeType() == SPHERE_SHAPE_PROXYTYPE)
				{
					glm::mat4 m;
					t->GetTransform(m);
					m = glm::scale(m, glm::vec3(t->GetRadius() * 2.0f));
					debugDrawMngr->AddSphere(m, glm::vec3(1.0f, 1.0f, 0.0f));
				}
				else if (t->GetShapeType() == CAPSULE_SHAPE_PROXYTYPE)
				{
					/*float radius = t->GetRadius();
					float height = t->GetHeight();

					glm::mat4 m = glm::translate(glm::mat4(1.0f), glm::vec3(t->GetPosition() + t->GetCenter()));
					m = glm::scale(m, glm::vec3(radius * 2.0f, height, radius * 2.0f));
					debugDrawMngr->AddCapsule(m);*/
				}
			}
		}

		for (size_t i = 0; i < usedRigidBodies; i++)
		{
			RigidBody *rb = rigidBodies[i].rb;
			if (rb->WantsDebugView())
			{
				if (rb->GetShapeType() == BOX_SHAPE_PROXYTYPE)
				{
					glm::mat4 m;
					rb->GetTransform(m);
					m = glm::scale(m, rb->GetBoxSize());
					debugDrawMngr->AddCube(m, glm::vec3(0.0f, 1.0f, 1.0f));			// Add rigid bodies as blue
				}
				else if (rb->GetShapeType() == SPHERE_SHAPE_PROXYTYPE)
				{
					glm::mat4 m;
					rb->GetTransform(m);
					m = glm::scale(m, glm::vec3(rb->GetRadius() * 2.0f));
					debugDrawMngr->AddSphere(m, glm::vec3(0.0f, 1.0f, 1.0f));
				}
				else if (rb->GetShapeType() == CAPSULE_SHAPE_PROXYTYPE)
				{
					float radius = rb->GetRadius();
					float diameter = radius * 2.0f;
					float height = rb->GetHeight();

					glm::mat4 m;
					rb->GetTransform(m);

					m = glm::scale(m, glm::vec3(diameter, height, diameter));
					debugDrawMngr->AddSphere(m, glm::vec3(0.0f, 1.0f, 1.0f));		// Replace with capsule
				}
			}
		}

		for (size_t i = 0; i < usedColliders; i++)
		{
			Collider *c = colliders[i].col;
			if (c->WantsDebugView())
			{
				if (c->GetShapeType() == BOX_SHAPE_PROXYTYPE)
				{
					glm::mat4 m;
					c->GetTransform(m);
					m = glm::scale(m, c->GetBoxSize());
					debugDrawMngr->AddCube(m, glm::vec3(0.0f, 1.0f, 0.0f));			// Render colliders as green
				}
				else if (c->GetShapeType() == SPHERE_SHAPE_PROXYTYPE)
				{
					glm::mat4 m;
					c->GetTransform(m);
					m = glm::scale(m, glm::vec3(c->GetRadius() * 2.0f));
					debugDrawMngr->AddSphere(m, glm::vec3(0.0f, 1.0f, 0.0f));
				}
				else if (c->GetShapeType() == CAPSULE_SHAPE_PROXYTYPE)
				{
					/*float radius = collider->GetRadius();
					float height = collider->GetHeight();

					glm::mat4 m = glm::translate(glm::mat4(1.0f), glm::vec3(GetLocalToWorldTransform()[3]));
					m = glm::scale(m, glm::vec3(radius * 2.0f, height, radius * 2.0f));
					renderer->SubmitDebug(MeshType::SPHERE, m);*/
				}
			}
		}
	}

	void PhysicsManager::PartialDispose()
	{
		for (size_t i = 0; i < usedRigidBodies; i++)
		{
			btRigidBody *rb = rigidBodies[i].rb->GetHandle();
			dynamicsWorld->removeRigidBody(rb);
			delete rb->getMotionState();
			delete rb;
			rb = nullptr;

			delete rigidBodies[i].rb;
			rigidBodies[i].rb = nullptr;
		}

		for (size_t i = 0; i < usedColliders; i++)
		{
			btCollisionObject *col = colliders[i].col->GetHandle();
			dynamicsWorld->removeCollisionObject(col);
			delete col;
			col = nullptr;

			delete colliders[i].col;
			colliders[i].col = nullptr;
		}

		for (size_t i = 0; i < usedTriggers; i++)
		{
			//btGhostObject *ghost = triggers[i]->GetHandle();
			btPairCachingGhostObject *ghost = triggers[i].tr->GetHandle();
			dynamicsWorld->removeCollisionObject(ghost);
			delete ghost;
			ghost = nullptr;
			delete triggers[i].tr;
			triggers[i].tr = nullptr;
		}

		for (size_t i = 0; i < collisionShapes.size(); i++)
		{
			delete collisionShapes[i];
			collisionShapes[i] = nullptr;
		}

		if (terrainCollider)
		{
			dynamicsWorld->removeCollisionObject(terrainCollider);
			delete terrainCollider;
			terrainCollider = nullptr;
		}

		for (size_t i = 0; i < terrainShapes.size(); i++)
		{
			delete terrainShapes[i];
			terrainShapes[i] = nullptr;
		}

		rigidBodies.clear();
		colliders.clear();
		triggers.clear();
		collisionShapes.clear();
		terrainShapes.clear();
	}

	void PhysicsManager::Dispose()
	{
		if (!isInit)
			return;

		PartialDispose();

		if (ghostCallback)
			delete ghostCallback;
		if (dynamicsWorld)
			delete dynamicsWorld;
		if (solver)
			delete solver;
		if (dispatcher)
			delete dispatcher;
		if (collisionConfiguration)
			delete collisionConfiguration;
		if (broadphase)
			delete broadphase;

		isInit = false;
	}

	bool PhysicsManager::HasRigidBody(Entity e) const
	{
		return rbMap.find(e.id) != rbMap.end();
	}

	bool PhysicsManager::HasTrigger(Entity e) const
	{
		return  trMap.find(e.id) != trMap.end();
	}

	bool PhysicsManager::HasCollider(Entity e) const
	{
		return  colMap.find(e.id) != colMap.end();
	}

	void PhysicsManager::DuplicateRigidBody(Entity e, Entity newE)
	{
		if (HasRigidBody(e) == false)
			return;

		const RigidBody *rb = GetRigidBody(e);

		btCollisionShape *shape = nullptr;

		if (rb->GetShapeType() == BOX_SHAPE_PROXYTYPE)
		{
			glm::vec3 boxSize = rb->GetBoxSize();
			shape = GetBoxShape(btVector3(boxSize.x * 0.5f, boxSize.y * 0.5f, boxSize.z * 0.5f));
		}
		else if (rb->GetShapeType() == SPHERE_SHAPE_PROXYTYPE)
		{
			shape = GetSphereShape(rb->GetRadius());
		}
		else if (rb->GetShapeType() == CAPSULE_SHAPE_PROXYTYPE)
		{
			// The height parameter is actually the capsule's cylinder height and not the full height (which also includes the radius of both spheres)
			// So compute just the cylinder height
			float radius = rb->GetRadius();

			float cylinderHeight = std::abs(rb->GetHeight() - 2 * radius);
			shape = GetCapsuleShape(radius, cylinderHeight);
		}

		if (!shape)
			return;

		RigidBody *newRB = new RigidBody(shape, rb->GetMass());
		newRB->DebugView(rb->WantsDebugView());
		newRB->SetKinematic(rb->IsKinematic());
		newRB->SetRestitution(rb->GetRestitution());
		newRB->SetAngularFactor(rb->GetAngularFactor());
		glm::vec3 linearFactor = rb->GetLinearFactor();
		newRB->SetLinearFactor(linearFactor.x, linearFactor.y, linearFactor.z);
		newRB->GetHandle()->setSleepingThresholds(rb->GetHandle()->getLinearSleepingThreshold(), rb->GetHandle()->getAngularSleepingThreshold());
		newRB->SetCenter(rb->GetCenter());
		newRB->GetHandle()->setFriction(rb->GetHandle()->getFriction());

		newRB->GetHandle()->setUserIndex((int)newE.id);

		dynamicsWorld->addRigidBody(newRB->GetHandle(), Layer::DEFAULT, Layer::DEFAULT | Layer::OBSTACLE | Layer::TERRAIN | Layer::ENEMY);

		RigidBodyInstance rbi = {};
		rbi.e = newE;
		rbi.rb = newRB;

		if (usedRigidBodies < rigidBodies.size())
		{
			rigidBodies[usedRigidBodies] = rbi;
			rbMap[newE.id] = usedRigidBodies;
		}
		else
		{
			rigidBodies.push_back(rbi);
			rbMap[newE.id] = (unsigned int)rigidBodies.size() - 1;
		}

		usedRigidBodies++;
	}

	void PhysicsManager::DuplicateCollider(Entity e, Entity newE)
	{
		if (HasCollider(e) == false)
			return;

		const Collider *col = GetCollider(e);

		btCollisionShape *shape = nullptr;

		if (col->GetShapeType() == BOX_SHAPE_PROXYTYPE)
		{
			glm::vec3 boxSize = col->GetBoxSize();
			shape = GetBoxShape(btVector3(boxSize.x * 0.5f, boxSize.y * 0.5f, boxSize.z * 0.5f));
		}
		else if (col->GetShapeType() == SPHERE_SHAPE_PROXYTYPE)
		{
			shape = GetSphereShape(col->GetRadius());
		}
		else if (col->GetShapeType() == CAPSULE_SHAPE_PROXYTYPE)
		{
			// The height parameter is actually the capsule's cylinder height and not the full height (which also includes the radius of both spheres)
			// So compute just the cylinder height
			float radius = col->GetRadius();
			float cylinderHeight = std::abs(col->GetHeight() - 2 * radius);
			shape = GetCapsuleShape(radius, cylinderHeight);
		}

		if (!shape)
			return;

		btCollisionObject *colObject = new btCollisionObject();
		//colObject->getWorldTransform().setOrigin(col->G);
		//colObject->setWorldTransform(col->collider->getWorldTransform());
		colObject->setCollisionShape(shape);
		colObject->setFriction(col->GetHandle()->getFriction());

		colObject->setUserIndex((int)newE.id);

		dynamicsWorld->addCollisionObject(colObject, Layer::DEFAULT, Layer::DEFAULT | Layer::OBSTACLE | Layer::ENEMY);

		Collider *newCol = new Collider(colObject, shape);
		//newCol->SetTransform(col->GetTransform());
		newCol->SetCenter(col->GetCenter());	

		ColliderInstance ci = {};
		ci.e = newE;
		ci.col = newCol;

		if (usedColliders < colliders.size())
		{
			colliders[usedColliders] = ci;
			colMap[newE.id] = usedColliders;
		}
		else
		{
			colliders.push_back(ci);
			colMap[newE.id] = (unsigned int)colliders.size() - 1;
		}

		usedColliders++;
	}

	void PhysicsManager::DuplicateTrigger(Entity e, Entity newE)
	{
		if (HasTrigger(e) == false)
			return;

		const Trigger *tr = GetTrigger(e);

		btCollisionShape *shape = nullptr;

		if (tr->GetShapeType() == BOX_SHAPE_PROXYTYPE)
		{
			glm::vec3 boxSize = tr->GetBoxSize();
			shape = GetBoxShape(btVector3(boxSize.x * 0.5f, boxSize.y*0.5f, boxSize.z*0.5f));
		}
		else if (tr->GetShapeType() == SPHERE_SHAPE_PROXYTYPE)
		{
			shape = GetSphereShape(tr->GetRadius());
		}
		else if (tr->GetShapeType() == CAPSULE_SHAPE_PROXYTYPE)
		{
			float radius = tr->GetRadius();
			// The height parameter is actually the capsule's cylinder height and not the full height (which also includes the radius of both spheres)
			// So compute just the cylinder height
			float cylinderHeight = std::abs(tr->GetHeight() - 2 * radius);
			shape = GetCapsuleShape(radius, cylinderHeight);
		}

		if (!shape)
			return;

		const glm::vec3 &worldPos = transformManager->GetLocalToWorld(newE)[3];
		btTransform t(btQuaternion(0.0f, 0.0f, 0.0f, 1.0f), btVector3(worldPos.x, worldPos.y, worldPos.z));

		btPairCachingGhostObject *ghost = new btPairCachingGhostObject();
		ghost->setCollisionShape(shape);
		ghost->setWorldTransform(t);
		ghost->setCollisionFlags(ghost->getCollisionFlags() | btCollisionObject::CF_NO_CONTACT_RESPONSE);

		ghost->setUserIndex((int)newE.id);

		// Ghost objects colliding with the terrain were causing a huge lag. Terrain is now on a separate layer and doesn't work with triggers
		dynamicsWorld->addCollisionObject(ghost, Layer::DEFAULT, Layer::DEFAULT | Layer::OBSTACLE | Layer::ENEMY);

		Trigger *newTr = new Trigger(this, ghost);
		newTr->SetCenter(transformManager->GetLocalToWorld(newE), tr->GetCenter());	

		TriggerInstance ti = {};
		ti.e = newE;
		ti.tr = newTr;

		if (usedTriggers < triggers.size())
		{
			triggers[usedTriggers] = ti;
			trMap[newE.id] = usedTriggers;
		}
		else
		{
			triggers.push_back(ti);
			trMap[newE.id] = (unsigned int)triggers.size() - 1;
		}

		usedTriggers++;
	}

	void PhysicsManager::RemoveRigidBody(Entity e)
	{
		if (HasRigidBody(e))
		{
			unsigned int index = rbMap.at(e.id);

			RigidBodyInstance temp = rigidBodies[index];
			RigidBodyInstance last = rigidBodies[rigidBodies.size() - 1];
			rigidBodies[rigidBodies.size() - 1] = temp;
			rigidBodies[index] = last;

			rbMap[last.e.id] = index;
			rbMap.erase(e.id);

			btRigidBody *rigidBody = temp.rb->GetHandle();
			dynamicsWorld->removeRigidBody(rigidBody);
			delete rigidBody->getMotionState();
			delete rigidBody;
			delete temp.rb;
			usedRigidBodies--;
		}
	}

	void PhysicsManager::RemoveCollider(Entity e)
	{
		if (HasCollider(e))
		{
			unsigned int index = colMap.at(e.id);

			ColliderInstance temp = colliders[index];
			ColliderInstance last = colliders[colliders.size() - 1];
			colliders[colliders.size() - 1] = temp;
			colliders[index] = last;

			colMap[last.e.id] = index;
			colMap.erase(e.id);

			btCollisionObject *collider = temp.col->GetHandle();
			dynamicsWorld->removeCollisionObject(collider);
			delete collider;
			delete temp.col;
			usedColliders--;
		}
	}

	void PhysicsManager::RemoveTrigger(Entity e)
	{
		if (HasTrigger(e))
		{
			unsigned int index = trMap.at(e.id);

			TriggerInstance temp = triggers[index];
			TriggerInstance last = triggers[triggers.size() - 1];
			triggers[triggers.size() - 1] = temp;
			triggers[index] = last;

			trMap[last.e.id] = index;
			trMap.erase(e.id);

			//btGhostObject *ghostTrigger = ghost->GetHandle();
			btPairCachingGhostObject *ghostTrigger = temp.tr->GetHandle();
			dynamicsWorld->removeCollisionObject(ghostTrigger);
			delete ghostTrigger;
			delete temp.tr;
			usedTriggers--;
		}
	}

	RigidBody *PhysicsManager::AddBoxRigidBody(Entity e, const btVector3 &halfExtents, float mass, int layer)
	{
		if (rbMap.find(e.id) != rbMap.end())
			return GetRigidBody(e);

		btCollisionShape *shape = GetBoxShape(halfExtents);

		RigidBody *rb = new RigidBody(shape, mass);
		dynamicsWorld->addRigidBody(rb->GetHandle(), layer, Layer::DEFAULT | Layer::OBSTACLE | Layer::TERRAIN | Layer::ENEMY);

		rb->GetHandle()->setUserIndex((int)e.id);

		RigidBodyInstance rbi = {};
		rbi.e = e;
		rbi.rb = rb;

		if (usedRigidBodies < rigidBodies.size())
		{
			rigidBodies[usedRigidBodies] = rbi;
			rbMap[e.id] = usedRigidBodies;
		}
		else
		{
			rigidBodies.push_back(rbi);
			rbMap[e.id] = (unsigned int)rigidBodies.size() - 1;
		}

		usedRigidBodies++;

		return rb;
	}

	RigidBody *PhysicsManager::AddSphereRigidBody(Entity e, float radius, float mass, int layer)
	{
		if (rbMap.find(e.id) != rbMap.end())
			return GetRigidBody(e);

		btCollisionShape *shape = GetSphereShape(radius);

		RigidBody *rb = new RigidBody(shape, mass);
		dynamicsWorld->addRigidBody(rb->GetHandle(), layer, Layer::DEFAULT | Layer::OBSTACLE | Layer::TERRAIN | Layer::ENEMY);
		
		rb->GetHandle()->setUserIndex((int)e.id);

		RigidBodyInstance rbi = {};
		rbi.e = e;
		rbi.rb = rb;

		if (usedRigidBodies < rigidBodies.size())
		{
			rigidBodies[usedRigidBodies] = rbi;
			rbMap[e.id] = usedRigidBodies;
		}
		else
		{
			rigidBodies.push_back(rbi);
			rbMap[e.id] = (unsigned int)rigidBodies.size() - 1;
		}		

		usedRigidBodies++;

		return rb;
	}

	RigidBody *PhysicsManager::AddCapsuleRigidBody(Entity e, float radius, float height, float mass, int layer)
	{
		if (rbMap.find(e.id) != rbMap.end())
			return GetRigidBody(e);

		// The height parameter is actually the capsule's cylinder height and not the full height (which also includes the radius of both spheres)
		// So compute just the cylinder height
		float cylinderHeight = std::abs(height - 2 * radius);

		btCollisionShape *shape = GetCapsuleShape(radius, cylinderHeight);

		RigidBody *rb = new RigidBody(shape, mass);
		dynamicsWorld->addRigidBody(rb->GetHandle(), layer, Layer::DEFAULT | Layer::OBSTACLE | Layer::TERRAIN | Layer::ENEMY);
		
		rb->GetHandle()->setUserIndex((int)e.id);

		RigidBodyInstance rbi = {};
		rbi.e = e;
		rbi.rb = rb;

		if (usedRigidBodies < rigidBodies.size())
		{
			rigidBodies[usedRigidBodies] = rbi;
			rbMap[e.id] = usedRigidBodies;
		}
		else
		{
			rigidBodies.push_back(rbi);
			rbMap[e.id] = (unsigned int)rigidBodies.size() - 1;
		}

		usedRigidBodies++;

		return rb;
	}

	btCollisionShape *PhysicsManager::GetBoxShape(const btVector3 &halfExtents)
	{
		// If we're using the editor we don't want to share shapes because we might be editing an object's shape and like this every object that uses the same shape would get its shape changed
#ifndef EDITOR
	// First try to find a shape that is approximatly equal to the one we want
		for (size_t i = 0; i < collisionShapes.size(); i++)
		{
			if (collisionShapes[i]->getShapeType() == BOX_SHAPE_PROXYTYPE)
			{
				btVector3 halfExts = static_cast<btBoxShape*>(collisionShapes[i])->getHalfExtentsWithMargin();
				if (glm::abs(halfExtents.x() - halfExts.x()) < 0.02f)
				{
					if (glm::abs(halfExtents.y() - halfExts.y()) < 0.02f)
					{
						if (glm::abs(halfExtents.z() - halfExts.z()) < 0.02f)
						{
							return collisionShapes[i];
						}
					}
				}
			}
		}
#endif
		// Else if we didn't find a shape, create a new one
		btCollisionShape *boxShape = new btBoxShape(halfExtents);
		collisionShapes.push_back(boxShape);
		return boxShape;
	}

	btCollisionShape *PhysicsManager::GetSphereShape(float radius)
	{
		// If we're using the editor we don't want to share shapes because we might be editing an object's shape and like this every object that uses the same shape would get its shape changed
#ifndef EDITOR
	// First try to find a shape that is approximatly equal to the one we want
		for (size_t i = 0; i < collisionShapes.size(); i++)
		{
			if (collisionShapes[i]->getShapeType() == SPHERE_SHAPE_PROXYTYPE)
			{
				float r = static_cast<btSphereShape*>(collisionShapes[i])->getRadius();
				if (glm::abs(radius - r) < 0.02f)
				{
					return collisionShapes[i];
				}
			}
		}
#endif
		// Else we didn't find a shape so create a new one
		btCollisionShape *sphereShape = new btSphereShape(radius);
		collisionShapes.push_back(sphereShape);
		return sphereShape;
	}

	btCollisionShape *PhysicsManager::GetCapsuleShape(float radius, float height)
	{
		// If we're using the editor we don't want to share shapes because we might be editing an object's shape and like this every object that uses the same shape would get its shape changed
#ifndef EDITOR
	// First try to find a shape that is approximatly equal to the one we want
		for (size_t i = 0; i < collisionShapes.size(); i++)
		{
			if (collisionShapes[i]->getShapeType() == CAPSULE_SHAPE_PROXYTYPE)
			{
				btCapsuleShape *cap = static_cast<btCapsuleShape*>(collisionShapes[i]);
				float h = cap->getHalfHeight() * 2.0f;					// Use cylinder height instead of complete height
				float r = cap->getRadius();
				if (glm::abs(height - h) < 0.02f)
				{
					if (glm::abs(radius - r) < 0.02f)
					{
						return collisionShapes[i];
					}
				}
			}
		}
#endif
		// Else we didn't find a shape so create a new one
		btCollisionShape *capsuleShape = new btCapsuleShape(radius, height);
		collisionShapes.push_back(capsuleShape);
		return capsuleShape;
	}

	Collider *PhysicsManager::AddBoxCollider(Entity e, const btVector3 &center, const btVector3 &halfExtents, int layer)
	{
		if (colMap.find(e.id) != colMap.end())
			return GetCollider(e);

		btCollisionShape *box = GetBoxShape(halfExtents);

		btCollisionObject *colObject = new btCollisionObject();
		colObject->getWorldTransform().setOrigin(center);
		colObject->setCollisionShape(box);
		colObject->setUserIndex((int)e.id);

		dynamicsWorld->addCollisionObject(colObject, layer, Layer::DEFAULT | Layer::OBSTACLE | Layer::ENEMY);

		Collider *col = new Collider(colObject, box);

		ColliderInstance ci = {};
		ci.e = e;
		ci.col = col;

		if (usedColliders < colliders.size())
		{
			colliders[usedColliders] = ci;
			colMap[e.id] = usedColliders;
		}
		else
		{
			colliders.push_back(ci);
			colMap[e.id] = (unsigned int)colliders.size() - 1;
		}

		usedColliders++;

		return col;
	}

	Collider *PhysicsManager::AddSphereCollider(Entity e, const btVector3 &center, float radius, int layer)
	{
		if (colMap.find(e.id) != colMap.end())
			return GetCollider(e);

		btCollisionShape *sphere = GetSphereShape(radius);

		btCollisionObject *colObject = new btCollisionObject();
		colObject->getWorldTransform().setOrigin(center);
		colObject->setCollisionShape(sphere);
		colObject->setUserIndex((int)e.id);

		dynamicsWorld->addCollisionObject(colObject, layer, Layer::DEFAULT | Layer::OBSTACLE | Layer::ENEMY);

		Collider *col = new Collider(colObject, sphere);

		ColliderInstance ci = {};
		ci.e = e;
		ci.col = col;

		if (usedColliders < colliders.size())
		{
			colliders[usedColliders] = ci;
			colMap[e.id] = usedColliders;
		}
		else
		{
			colliders.push_back(ci);
			colMap[e.id] = (unsigned int)colliders.size() - 1;
		}

		usedColliders++;

		return col;
	}

	Collider *PhysicsManager::AddCapsuleCollider(Entity e, const btVector3 &center, float radius, float height, int layer)
	{
		if (colMap.find(e.id) != colMap.end())
			return GetCollider(e);

		// The height parameter is actually the capsule's cylinder height and not the full height (which also includes the radius of both spheres)
		// So compute just the cylinder height
		float cylinderHeight = std::abs(height - 2 * radius);

		btCollisionShape *capsule = GetCapsuleShape(radius, cylinderHeight);

		btCollisionObject *colObject = new btCollisionObject();
		colObject->getWorldTransform().setOrigin(center);
		colObject->setCollisionShape(capsule);
		colObject->setUserIndex((int)e.id);

		dynamicsWorld->addCollisionObject(colObject, layer, Layer::DEFAULT | Layer::OBSTACLE | Layer::ENEMY);

		Collider *col = new Collider(colObject, capsule);

		ColliderInstance ci = {};
		ci.e = e;
		ci.col = col;

		if (usedColliders < colliders.size())
		{
			colliders[usedColliders] = ci;
			colMap[e.id] = usedColliders;
		}
		else
		{
			colliders.push_back(ci);
			colMap[e.id] = (unsigned int)colliders.size() - 1;
		}

		usedColliders++;

		return col;
	}

	Collider *PhysicsManager::AddBoxCollider(const btVector3 &center, const btVector3 &halfExtents, int layer)
	{
		btCollisionShape *box = GetBoxShape(halfExtents);

		btCollisionObject *colObject = new btCollisionObject();
		colObject->getWorldTransform().setOrigin(center);
		colObject->setCollisionShape(box);

		dynamicsWorld->addCollisionObject(colObject, layer, Layer::DEFAULT | Layer::OBSTACLE | Layer::ENEMY);

		Collider *col = new Collider(colObject, box);

		ColliderInstance ci = {};
		ci.e = { std::numeric_limits<unsigned int>::max() };
		ci.col = col;

		if (usedColliders < colliders.size())
			colliders[usedColliders] = ci;
		else
			colliders.push_back(ci);

		usedColliders++;

		return col;
	}

	int PhysicsManager::AddTerrainCollider(int shapeID, int resolution, const void *data, float maxHeight)
	{
		if (shapeID >= 0 && shapeID < (int)terrainShapes.size())
			return RecreateTerrainShape(shapeID, resolution, data, maxHeight);

		// Height scale is ignored when using type float
		btCollisionShape *shape = new btHeightfieldTerrainShape(resolution, resolution, data, 1.0f, 0.0f, maxHeight, 1, PHY_FLOAT, true);
		terrainShapes.push_back(shape);

		// We have to recenter the terrain because bullet places it at the origin
		// E.g. Move it from [-128,128] to [0,256]
		float halfRes = (float)resolution * 0.5f;
		float halfHeight = maxHeight * 0.5f;

		terrainCollider = new btCollisionObject();
		terrainCollider->getWorldTransform().setOrigin(btVector3(halfRes, halfHeight, halfRes));
		terrainCollider->setCollisionShape(shape);
		terrainCollider->setUserPointer(nullptr);
		terrainCollider->setRestitution(0.0f);
		//terrainCollider->setRollingFriction(1.0f);
		terrainCollider->setSpinningFriction(0.5f);
		//terrainCollider->setCollisionFlags(terrainCollider->getCollisionFlags() | btCollisionObject::CF_CUSTOM_MATERIAL_CALLBACK);

		// The terrain has it's own layer
		dynamicsWorld->addCollisionObject(terrainCollider, Layer::TERRAIN, Layer::DEFAULT | Layer::OBSTACLE | Layer::ENEMY);

		return terrainShapes.size() - 1;
	}

	int PhysicsManager::RecreateTerrainShape(int shapeID, int newResolution, const void *newData, float newMaxHeight)
	{
		delete terrainShapes[shapeID];
		terrainShapes[shapeID] = nullptr;

		btHeightfieldTerrainShape *newShape = new btHeightfieldTerrainShape(newResolution, newResolution, newData, 1.0f, 0.0f, newMaxHeight, 1, PHY_FLOAT, true);
		terrainShapes[shapeID] = newShape;

		float halfRes = (float)newResolution * 0.5f;
		float halfHeight = newMaxHeight * 0.5f;

		terrainCollider->getWorldTransform().setOrigin(btVector3(halfRes, halfHeight, halfRes));

		return shapeID;
	}

	Trigger *PhysicsManager::AddBoxTrigger(Entity e, const btVector3 &center, const btVector3 &halfExtents, int layer)
	{
		if (trMap.find(e.id) != trMap.end())
			return GetTrigger(e);

		btCollisionShape *boxShape = GetBoxShape(halfExtents);

		btTransform t(btQuaternion(0.0f, 0.0f, 0.0f, 1.0f), center);

		btPairCachingGhostObject *ghost = new btPairCachingGhostObject();
		ghost->setCollisionShape(boxShape);
		ghost->setWorldTransform(t);
		ghost->setCollisionFlags(ghost->getCollisionFlags() | btCollisionObject::CF_NO_CONTACT_RESPONSE);
		ghost->setUserIndex((int)e.id);

		// Ghost objects colliding with the terrain were causing a huge lag. Terrain is now on a separate layer and doesn't work with triggers
		dynamicsWorld->addCollisionObject(ghost, Layer::DEFAULT, Layer::DEFAULT | Layer::OBSTACLE | Layer::ENEMY);

		Trigger *tr = new Trigger(this, ghost);

		TriggerInstance ti = {};
		ti.e = e;
		ti.tr = tr;
	
		if (usedTriggers < triggers.size())
		{
			triggers[usedTriggers] = ti;
			trMap[e.id] = usedTriggers;
		}
		else
		{
			triggers.push_back(ti);
			trMap[e.id] = (unsigned int)triggers.size() - 1;
		}

		usedTriggers++;

		return tr;
	}

	Trigger *PhysicsManager::AddSphereTrigger(Entity e, const btVector3 &center, float radius, int layer)
	{
		if (trMap.find(e.id) != trMap.end())
			return GetTrigger(e);

		btCollisionShape *sphere = GetSphereShape(radius);
		collisionShapes.push_back(sphere);

		btTransform t(btQuaternion(0.0f, 0.0f, 0.0f, 1.0f), center);

		btPairCachingGhostObject *ghost = new btPairCachingGhostObject();
		ghost->setCollisionShape(sphere);
		ghost->setWorldTransform(t);
		ghost->setCollisionFlags(ghost->getCollisionFlags() | btCollisionObject::CF_NO_CONTACT_RESPONSE);
		ghost->setUserIndex((int)e.id);

		// Ghost objects colliding with the terrain were causing a huge lag. Terrain is now on a separate layer and doesn't work with triggers
		dynamicsWorld->addCollisionObject(ghost, Layer::DEFAULT, Layer::DEFAULT | Layer::OBSTACLE | Layer::ENEMY);

		Trigger *tr = new Trigger(this, ghost);

		TriggerInstance ti = {};
		ti.e = e;
		ti.tr = tr;

		if (usedTriggers < triggers.size())
		{
			triggers[usedTriggers] = ti;
			trMap[e.id] = usedTriggers;
		}
		else
		{
			triggers.push_back(ti);
			trMap[e.id] = (unsigned int)triggers.size() - 1;
		}

		usedTriggers++;

		return tr;
	}

	Trigger *PhysicsManager::AddCapsuleTrigger(Entity e, const btVector3 &center, float radius, float height, int layer)
	{
		if (trMap.find(e.id) != trMap.end())
			return GetTrigger(e);

		// The height parameter is actually the capsule's cylinder height and not the full height (which also includes the radius of both spheres)
		// So compute just the cylinder height
		float cylinderHeight = std::abs(height - 2 * radius);

		btCollisionShape *capsule = GetCapsuleShape(radius, cylinderHeight);
		collisionShapes.push_back(capsule);

		btTransform t(btQuaternion(0.0f, 0.0f, 0.0f, 1.0f), center);

		btPairCachingGhostObject *ghost = new btPairCachingGhostObject();
		ghost->setCollisionShape(capsule);
		ghost->setWorldTransform(t);
		ghost->setCollisionFlags(ghost->getCollisionFlags() | btCollisionObject::CF_NO_CONTACT_RESPONSE);
		ghost->setUserIndex((int)e.id);

		// Ghost objects colliding with the terrain were causing a huge lag. Terrain is now on a separate layer and doesn't work with triggers
		dynamicsWorld->addCollisionObject(ghost, Layer::DEFAULT, Layer::DEFAULT | Layer::OBSTACLE | Layer::ENEMY);

		Trigger *tr = new Trigger(this, ghost);

		TriggerInstance ti = {};
		ti.e = e;
		ti.tr = tr;

		if (usedTriggers < triggers.size())
		{
			triggers[usedTriggers] = ti;
			trMap[e.id] = usedTriggers;
		}
		else
		{
			triggers.push_back(ti);
			trMap[e.id] = (unsigned int)triggers.size() - 1;
		}

		usedTriggers++;

		return tr;
	}

	RigidBody *PhysicsManager::GetRigidBody(Entity e) const
	{
		return rigidBodies[rbMap.at(e.id)].rb;
	}

	Collider *PhysicsManager::GetCollider(Entity e) const
	{
		return colliders[colMap.at(e.id)].col;
	}

	Trigger *PhysicsManager::GetTrigger(Entity e) const
	{
		return triggers[trMap.at(e.id)].tr;
	}

	RigidBody *PhysicsManager::GetRigidBodySafe(Entity e) const
	{
		if (HasRigidBody(e))
			return rigidBodies[rbMap.at(e.id)].rb;

		return nullptr;
	}

	Collider *PhysicsManager::GetColliderSafe(Entity e) const
	{
		if (HasCollider(e))
			return colliders[colMap.at(e.id)].col;

		return nullptr;
	}

	Trigger *PhysicsManager::GetTriggerSafe(Entity e) const
	{
		if (HasTrigger(e))
			return triggers[trMap.at(e.id)].tr;

		return nullptr;
	}

	void PhysicsManager::ChangeLayer(RigidBody *rigidBody, int newLayer)
	{
		dynamicsWorld->removeRigidBody(rigidBody->GetHandle());
		dynamicsWorld->addRigidBody(rigidBody->GetHandle(), newLayer, Layer::DEFAULT | Layer::OBSTACLE | Layer::ENEMY | Layer::TERRAIN);
	}

	void PhysicsManager::ChangeLayer(Collider *col, int newLayer)
	{
		dynamicsWorld->removeCollisionObject(col->GetHandle());
		dynamicsWorld->addCollisionObject(col->GetHandle(), newLayer, Layer::DEFAULT | Layer::OBSTACLE | Layer::ENEMY);
	}

	//RaycastResult PhysicsManager::PerformRaycast(const Ray &ray, float maxRayDistance)
	RaycastResult PhysicsManager::PerformRaycast(const glm::vec3 &rayOrigin, const glm::vec3 &rayDir, float maxRayDistance)
	{
		glm::vec3 origin = rayOrigin;
		glm::vec3 end = origin + rayDir * maxRayDistance;

		btCollisionWorld::ClosestRayResultCallback rayCallback(btVector3(origin.x, origin.y, origin.z), btVector3(end.x, end.y, end.z));
		rayCallback.m_collisionFilterGroup = Layer::DEFAULT;
		rayCallback.m_collisionFilterMask = Layer::DEFAULT | Layer::OBSTACLE | Layer::TERRAIN;

		dynamicsWorld->rayTest(btVector3(origin.x, origin.y, origin.z), btVector3(end.x, end.y, end.z), rayCallback);

		RaycastResult r = {};

		if (rayCallback.hasHit())
		{
			r.e = { (unsigned int)rayCallback.m_collisionObject->getUserIndex() };
			r.hit = true;
			r.hitDistance = glm::length(origin - glm::vec3(rayCallback.m_hitPointWorld.x(), rayCallback.m_hitPointWorld.y(), rayCallback.m_hitPointWorld.z()));
		}
		else
		{
			r.e = { std::numeric_limits<unsigned int>::max() };
			r.hit = false;
		}

		return r;
	}

	/*btCollisionWorld::ClosestRayResultCallback PhysicsManager::PerformRaycast(const glm::vec3 &origin, const glm::vec3 &dir, float maxDistance)
	{
		glm::vec3 end = origin + dir * maxDistance;

		btCollisionWorld::ClosestRayResultCallback rayCallback(btVector3(origin.x, origin.y, origin.z), btVector3(end.x, end.y, end.z));
		rayCallback.m_collisionFilterGroup = Layer::DEFAULT;
		rayCallback.m_collisionFilterMask = Layer::DEFAULT | Layer::OBSTACLE;

		dynamicsWorld->rayTest(btVector3(origin.x, origin.y, origin.z), btVector3(end.x, end.y, end.z), rayCallback);

		return rayCallback;
	}*/

	bool PhysicsManager::CheckSphere(const btVector3 &center, float radius)
	{
		btSphereShape sphere(radius);
		btTransform t(btQuaternion(0.0f, 0.0f, 0.0f, 1.0f), center);
		ghost.setCollisionShape(&sphere);
		ghost.setWorldTransform(t);
		ghost.setCollisionFlags(ghost.getCollisionFlags() | btCollisionObject::CF_NO_CONTACT_RESPONSE | btCollisionObject::CF_STATIC_OBJECT);

		dynamicsWorld->addCollisionObject(&ghost, Layer::DEFAULT, Layer::OBSTACLE);
		bool overlaps = false;

		/*if (ghost.getNumOverlappingObjects() > 0)
			overlaps = true;*/

		if (ghost.getOverlappingPairCache()->getNumOverlappingPairs() > 0)
		{
			//std::cout << "Pairs: " << ghost.getOverlappingPairCache()->getNumOverlappingPairs() << '\n';
			//dynamicsWorld->getDispatcher()->dispatchAllCollisionPairs(ghost.getOverlappingPairCache(), dynamicsWorld->getDispatchInfo(), dynamicsWorld->getDispatcher());
			dynamicsWorld->stepSimulation(0.01f, 0);

			btManifoldArray manifoldArray;
			btBroadphasePairArray& pairArray = ghost.getOverlappingPairCache()->getOverlappingPairArray();
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
						overlaps = true;
						break;
					}
				}

				if (overlaps)
					break;
			}

		}
		
		dynamicsWorld->removeCollisionObject(&ghost);
		return overlaps;
	}

	void PhysicsManager::Serialize(Serializer &s) const
	{
		s.Write(usedRigidBodies);
		for (unsigned int i = 0; i < usedRigidBodies; i++)
		{
			const RigidBodyInstance &rbi = rigidBodies[i];
			s.Write(rbi.e.id);
			rbi.rb->Serialize(s);
		}
		s.Write(usedColliders);
		for (unsigned int i = 0; i < usedColliders; i++)
		{
			const ColliderInstance &ci = colliders[i];
			s.Write(ci.e.id);
			ci.col->Serialize(s);
		}
		s.Write(usedTriggers);
		for (unsigned int i = 0; i < usedTriggers; i++)
		{
			const TriggerInstance &ti = triggers[i];
			s.Write(ti.e.id);
			ti.tr->Serialize(s);
		}
	}

	void PhysicsManager::Deserialize(Serializer &s, bool reload)
	{
		if (!reload)
		{
			s.Read(usedRigidBodies);
			rigidBodies.resize(usedRigidBodies);
			for (unsigned int i = 0; i < usedRigidBodies; i++)
			{
				RigidBodyInstance rbi;
				s.Read(rbi.e.id);

				rbi.rb = new RigidBody();
				rbi.rb->Deserialize(*this, s);

				glm::mat4 m = transformManager->GetLocalToWorld(rbi.e);
				m[0] = glm::normalize(m[0]);				// Set scale to 1. Rigidbody size is set through it's own size property and not by the entity's scale
				m[1] = glm::normalize(m[1]);
				m[2] = glm::normalize(m[2]);

				rbi.rb->SetTransform(m);					// Otherwise it wouldn't be updated correctly
				rbi.rb->GetHandle()->setUserIndex((int)rbi.e.id);

				dynamicsWorld->addRigidBody(rbi.rb->GetHandle(), Layer::DEFAULT, Layer::ALL);		// Read the layer from file

				rigidBodies[i] = rbi;
				rbMap[rbi.e.id] = i;
			}
			s.Read(usedColliders);
			colliders.resize(usedColliders);
			for (unsigned int i = 0; i < usedColliders; i++)
			{
				ColliderInstance ci;
				s.Read(ci.e.id);

				ci.col = new Collider();
				ci.col->Deserialize(*this, s);

				glm::mat4 m = transformManager->GetLocalToWorld(ci.e);
				m[0] = glm::normalize(m[0]);				// Set scale to 1. Collider size is set through it's own size property and not by the entity's scale
				m[1] = glm::normalize(m[1]);
				m[2] = glm::normalize(m[2]);

				ci.col->SetTransform(m);		// Otherwise it wouldn't be updated correctly
				ci.col->GetHandle()->setUserIndex((int)ci.e.id);

				dynamicsWorld->addCollisionObject(ci.col->GetHandle(), Layer::DEFAULT, Layer::DEFAULT | Layer::OBSTACLE | Layer::ENEMY);

				colliders[i] = ci;
				colMap[ci.e.id] = i;
			}
			s.Read(usedTriggers);
			triggers.resize(usedTriggers);
			for (unsigned int i = 0; i < usedTriggers; i++)
			{
				TriggerInstance ti;
				s.Read(ti.e.id);

				ti.tr = new Trigger();
				ti.tr->Deserialize(*this, s);
				ti.tr->GetHandle()->setUserIndex((int)ti.e.id);
				ti.tr->SetTransform(transformManager->GetLocalToWorld(ti.e));			// Set the triggers transform otherwise it will cause all of them to call OnTriggerEnter,etc because they all get initialized at (0,0,0)

				dynamicsWorld->addCollisionObject(ti.tr->GetHandle(), Layer::DEFAULT, Layer::DEFAULT | Layer::OBSTACLE | Layer::ENEMY);

				triggers[i] = ti;
				trMap[ti.e.id] = i;
			}
		}
		else
		{
			s.Read(usedRigidBodies);
			for (unsigned int i = 0; i < usedRigidBodies; i++)
			{
				RigidBodyInstance &rbi = rigidBodies[i];
				s.Read(rbi.e.id);
				rbi.rb->Deserialize(*this, s);
				rbi.rb->SetTransform(transformManager->GetLocalToWorld(rbi.e));
			}
			s.Read(usedColliders);
			for (unsigned int i = 0; i < usedColliders; i++)
			{
				ColliderInstance &ci = colliders[i];
				s.Read(ci.e.id);
				ci.col->Deserialize(*this, s);
			}
			s.Read(usedTriggers);
			for (unsigned int i = 0; i < usedTriggers; i++)
			{
				TriggerInstance &ti = triggers[i];
				s.Read(ti.e.id);
				ti.tr->Deserialize(*this, s);
			}
		}
	}
}
