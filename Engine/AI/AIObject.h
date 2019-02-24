#pragma once

#include "Game/EntityManager.h"

namespace Engine
{
	enum AIState
	{
		IDLE,
		CHASING,
		INVESTIGATING,
		AI_STATE_COUNT
	};

	class Game;

	class AIObject
	{
	public:
		AIObject(Game *game);

		void UpdateInGame(float dt);

		void SetTarget(Entity e);
		void SetEyesOffset(const glm::vec3 &offset) { eyesOffset = offset; }
		void SetEyesRange(float range) { eyesRange = range; }
		void SetAttackRange(float range) { attackRange = range; }
		void SetAttackDelay(float delay) { attackDelay = delay; }
		void SetFieldOfView(float fov) { this->fov = fov; }
		void SetTargetPosition(const glm::vec3 &pos) { targetWorldPosition = pos; }
		void SetState(int state);
		void SetMoveSpeed(float moveSpeed) { maxMoveSpeed = moveSpeed; }
		void TurnTo(float angle);

		Entity GetTarget() const { return target; }
		const glm::vec3 &GetEyesOffset() const { return eyesOffset; }
		float GetEyesRange() const { return eyesRange; }
		float GetAttackRange() const { return attackRange; }
		float GetAttackDelay() const { return attackDelay; }
		float GetFieldOfView() const { return fov; }
		float GetMoveSpeed() const { return maxMoveSpeed; }
		int GetState() const { return (int)state; }

		void Serialize(Serializer &s);
		void Deserialize(Serializer &s);

		// Script functions
		//static AIObject *CastFromObject(Object *obj) { if (!obj || obj->GetType() != ObjectType::AI_OBJECT) return nullptr; else { return static_cast<AIObject*>(obj); } }

	private:
		Game *game;
		Entity e;
		Entity target;
		glm::vec3 targetWorldPosition;		// The target position is separated from the object target because we might want the ai to go the other places and not just the target object
		glm::vec3 eyesOffset;
		float eyesRange;
		float attackRange;
		float fov;
		bool targetInFOV;

		std::vector<glm::vec2> pathWaypoints;
		bool followPath;
		float moveSpeed;
		float maxMoveSpeed;
		bool turning;
		float turnSpeed;
		float goalAngle;
		bool targetInSight;

		float castSightRayTimer;
		float castSightRayInterval;

		float idleTimer;
		float idleTimerMax;

		float requestPathTimer;

		float attackDelay;
		float attackTimer;
		bool attacking;

		bool targetSeenCalled;

		bool pathEnded;

		AIState state;
	};
}
