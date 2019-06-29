#pragma once

#include "Physics/Ray.h"
#include "Game/EntityManager.h"

#include "include/bullet/btBulletDynamicsCommon.h"
#include "include/bullet/btBulletCollisionCommon.h"
#include "include/bullet/BulletCollision/CollisionDispatch/btGhostObject.h"

#include <vector>
#include <unordered_map>

namespace Engine
{
	class RigidBody;
	class Collider;
	class Trigger;
	class DebugDrawManager;
	class TransformManager;
	class Game;

	struct RigidBodyInstance
	{
		Entity e;
		RigidBody *rb;
	};
	struct ColliderInstance
	{
		Entity e;
		Collider *col;
	};
	struct TriggerInstance
	{
		Entity e;
		Trigger *tr;
	};

	struct RaycastResult
	{
		Entity e;
		bool hit;
		float hitDistance;
	};

	enum class ShapeType
	{
		NONE,
		BOX,
		SPHERE,
		CAPSULE
	};

	enum Layer
	{
		DEFAULT = 64,		// Starts at 64 because bullet already has 6 layers
		OBSTACLE = 128,
		TERRAIN = 256,
		STATIC = 512,		// Useful for colliders serving as map limits for not colliding with the terrain
		ENEMY = 1024,
		ALL = DEFAULT | OBSTACLE | TERRAIN | STATIC | ENEMY
	};

	class PhysicsManager
	{
	public:
		PhysicsManager();

		void Init(Game *game);
		void Play();
		void Stop();
		void Simulate(float dt);
		void Update();	
		void PrepareDebugDraw(DebugDrawManager *debugDrawMngr);
		void PartialDispose();
		void Dispose();

		bool HasRigidBody(Entity e) const;
		bool HasTrigger(Entity e) const;
		bool HasCollider(Entity e) const;

		// Rigid bodies
		RigidBody *AddBoxRigidBody(Entity e, const btVector3 &halfExtents, float mass, int layer = Layer::DEFAULT);
		RigidBody *AddSphereRigidBody(Entity e, float radius, float mass, int layer = Layer::DEFAULT);
		RigidBody *AddCapsuleRigidBody(Entity e, float radius, float height, float mass, int layer = Layer::DEFAULT);

		// Colliders
		Collider *AddBoxCollider(Entity e, const btVector3 &center, const btVector3 &halfExtents, int layer = Layer::DEFAULT);
		Collider *AddSphereCollider(Entity e, const btVector3 &center, float radius, int layer = Layer::DEFAULT);
		Collider *AddCapsuleCollider(Entity e, const btVector3 &center, float radius, float height, int layer = Layer::DEFAULT);

		Collider *AddBoxCollider(const btVector3 &center, const btVector3 &halfExtents, int layer = Layer::DEFAULT);

		// Triggers
		Trigger *AddBoxTrigger(Entity e, const btVector3 &center, const btVector3 &halfExtents, int layer = Layer::DEFAULT);
		Trigger *AddSphereTrigger(Entity e, const btVector3 &center, float radius, int layer = Layer::DEFAULT);
		Trigger *AddCapsuleTrigger(Entity e, const btVector3 &center, float radius, float height, int layer = Layer::DEFAULT);

		size_t shapeCount() const { return collisionShapes.size(); }

		RigidBody *GetRigidBody(Entity e) const;
		Collider *GetCollider(Entity e) const;
		Trigger *GetTrigger(Entity e) const;

		RigidBody *GetRigidBodySafe(Entity e) const;
		Collider *GetColliderSafe(Entity e) const;
		Trigger *GetTriggerSafe(Entity e) const;

		void SetRigidBodyEnabled(Entity e, bool enable);
		void SetColliderEnabled(Entity e, bool enable);
		void SetTriggerEnabled(Entity e, bool enable);

		void DuplicateRigidBody(Entity e, Entity newE);
		void DuplicateCollider(Entity e, Entity newE);
		void DuplicateTrigger(Entity e, Entity newE);

		void LoadRigidBodyFromPrefab(Serializer &s, Entity e);
		void LoadColliderFromPrefab(Serializer &s, Entity e);
		void LoadTriggerFromPrefab(Serializer &s, Entity e);

		void RemoveRigidBody(Entity e);
		void RemoveCollider(Entity e);
		void RemoveTrigger(Entity e);

		btCollisionShape *GetBoxShape(const btVector3 &halfExtents);
		btCollisionShape *GetSphereShape(float radius);
		btCollisionShape *GetCapsuleShape(float radius, float height);

		// Add a new terrain collider. If shapeID is more or equal to 0 and less than terrainShapes.size, it replaces that terrain shape otherwise creates a new one
		int AddTerrainCollider(int shapeID, int resolution, const void *data, float maxHeight);				// Returns the id of the terrain shape

		void ChangeLayer(RigidBody *col, int newLayer);
		void ChangeLayer(Collider *col, int newLayer);

		//RaycastResult PerformRaycast(const Ray &ray, float maxRayDistance = 50.0f);
		RaycastResult PerformRaycast(const glm::vec3 &rayOrigin, const glm::vec3 &rayDir, float maxRayDistance);
		//btCollisionWorld::ClosestRayResultCallback PerformRaycast(const glm::vec3 &origin, const glm::vec3 &dir, float maxDistance = 50.0f);
		bool CheckSphere(const btVector3 &center, float radius);

		void Serialize(Serializer &s) const;
		void Deserialize(Serializer &s, bool reload = false);

	private:
		int RecreateTerrainShape(int shapeID, int newResolution, const void *newData, float newMaxHeight);

		void InsertRigidBodyInstance(const RigidBodyInstance &rbi);
		void InsertColliderInstance(const ColliderInstance &ci);
		void InsertTriggerInstance(const TriggerInstance &ti);

	private:
		bool isInit;
		Game *game;
		btBroadphaseInterface* broadphase;
		btDefaultCollisionConfiguration* collisionConfiguration;
		btCollisionDispatcher* dispatcher;
		btSequentialImpulseConstraintSolver* solver;
		btDiscreteDynamicsWorld* dynamicsWorld;

		btGhostPairCallback *ghostCallback;

		std::vector<btCollisionShape*> collisionShapes;

		TransformManager *transformManager;

		std::vector<RigidBodyInstance> rigidBodies;
		std::vector<TriggerInstance> triggers;
		std::vector<ColliderInstance> colliders;
		std::unordered_map<unsigned int, unsigned int> rbMap;
		std::unordered_map<unsigned int, unsigned int> trMap;
		std::unordered_map<unsigned int, unsigned int> colMap;
		unsigned int usedRigidBodies;
		unsigned int usedColliders;
		unsigned int usedTriggers;
		unsigned int disabledRigidBodies;
		unsigned int disabledColliders;
		unsigned int disabledTriggers;

		//btGhostObject ghost;
		//btPairCachingGhostObject ghost;		// Used to check overlaps to set the astar grid walkable areas

		//now only supporting one terrain collider
		btCollisionObject *terrainCollider;
		std::vector<btCollisionShape*> terrainShapes;
	};
}
