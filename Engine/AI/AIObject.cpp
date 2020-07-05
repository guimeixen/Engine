#include "AIObject.h"

#include "Game/Game.h"
#include "Program/Log.h"
#include "Program/Utils.h"

namespace Engine
{
	AIObject::AIObject(Game *game)
	{
		eyesOffset = glm::vec3(0.0f);
		eyesRange = 15.0f;
		attackRange = 1.0f;
		fov = 90.0f;
		targetInFOV = false;

		followPath = false;
		moveSpeed = 0.0f;
		maxMoveSpeed = 2.0f;
		turning = false;
		turnSpeed = 0.0f;
		goalAngle = 0.0f;
		targetInSight = false;

		pathEnded = false;

		castSightRayTimer = 0.0f;
		castSightRayInterval = 1.0f;			// A ray is casted every second to test for target visibility

		idleTimer = 0.0f;
		idleTimerMax = 4.0f;

		requestPathTimer = 0.0f;

		attackDelay = 1.0f;
		attackTimer = 0.0f;
		attacking = false;

		targetSeenCalled = false;

		state = IDLE;
	}

	void AIObject::UpdateInGame(float dt)
	{
		castSightRayTimer += dt;

		TransformManager &tm = game->GetTransformManager();

		glm::vec3 worldPosition = tm.GetWorldPosition(e);

		// First check if the target is in the ai's field of view
		glm::vec3 aiToTarget = glm::normalize(targetWorldPosition - worldPosition);
		glm::vec3 aiForward = glm::normalize(tm.GetLocalToWorld(e)[2]);			// Forward is stored in the third column of the world matrix. Also normalize it because it might have scaling applied
		if (glm::dot(aiToTarget, aiForward) >= glm::cos(glm::radians(fov * 0.5f)))
			targetInFOV = true;
		else
			targetInFOV = false;

		if (targetInFOV)
		{
			// If the target is in the FOV then update the target position to the target's position so the AI tries to chase the target
			targetWorldPosition = tm.GetWorldPosition(target);

			if (castSightRayTimer >= castSightRayInterval)
			{
				// We increase the height of where the ray leaves because the ai origin is at the feet and they can sometimes clip through the terrain which would
				// result in a ray being casted from below the terrain and would hit it from below and wouldn't get us any correct results
				//glm::vec3 raycastPos = worldPosition + eyesOffset;

				//btCollisionWorld::ClosestRayResultCallback rayCallback = game->GetPhysicsManager().PerformRaycast(raycastPos, glm::normalize(targetWorldPosition - raycastPos), eyesRange);

				targetInSight = false;
				
				/*if (rayCallback.hasHit())
				{*/
					//Object *obj = static_cast<Object*>(rayCallback.m_collisionObject->getUserPointer());

				/*	if (obj == nullptr)
					{
						//Log::Message("AI ray hit object is null.");
						targetSeenCalled = false;
					}
					else
					{*/
						state = CHASING;
						idleTimer = 0.0f;
						targetInSight = true;

						// Replace with entity
						/*if (!targetSeenCalled && script)
						{
							script->CallOnTargetSeen(target);
							targetSeenCalled = true;
						}*/
					}
				//}

				castSightRayTimer = 0.0f;
			//}
		}

		/*if (!targetInSight)
		{
			idleTimer += dt;

			if (idleTimer >= idleTimerMax)
			{
				state = IDLE;
			}
		}*/

		if (state == CHASING || state == INVESTIGATING)
		{
			requestPathTimer += dt;
		}

		if (requestPathTimer >= 1.0f)
		{
			//Log::Message("AI requested path.");
			game->GetAISystem().RequestPath(worldPosition, tm.GetWorldPosition(target), pathWaypoints);

			followPath = false;

			if (!pathWaypoints.empty())
			{
				followPath = true;
				pathEnded = false;
			}
			else if (glm::length2(worldPosition - tm.GetWorldPosition(target)) < 12.0f)
			{
				pathEnded = true;
			}

			requestPathTimer = 0.0f;
		}

		if ((state == CHASING || state == INVESTIGATING) && followPath)
		{
			glm::vec2 goal = pathWaypoints.back();

			glm::vec2 posDif = glm::vec2(goal.x - worldPosition.x, goal.y - worldPosition.z);

			TurnTo(utils::AngleFromPoint(posDif));

			glm::vec3 forward = glm::normalize(glm::vec3(posDif.x, 0.0f, posDif.y));
			// Move the enemy forward
			if (moveSpeed < maxMoveSpeed)
				moveSpeed += dt * 2.0f;

			tm.SetLocalPosition(e, worldPosition + forward * dt * moveSpeed);
			worldPosition = tm.GetWorldPosition(e);

			glm::vec2 dif = glm::abs(posDif);
			if (dif.x < 0.02f && dif.y < 0.02f)
			{
				pathWaypoints.pop_back();

				if (pathWaypoints.empty())
				{
					// Do a final rotation so the enemy faces the target
					glm::vec2 d = glm::vec2(targetWorldPosition.x - worldPosition.x, targetWorldPosition.z - worldPosition.z);
					TurnTo(utils::AngleFromPoint(d));

					followPath = false;
					pathEnded = true;
				}
			}
		}

		if (pathEnded)		// Move the monster forward torwards the player
		{
			worldPosition = tm.GetWorldPosition(e);
			glm::vec2 posDif = glm::vec2(targetWorldPosition.x - worldPosition.x, targetWorldPosition.z - worldPosition.z);
			glm::vec3 forward = glm::normalize(glm::vec3(posDif.x, 0.0f, posDif.y));
			worldPosition += forward * dt * moveSpeed;

			tm.SetLocalPosition(e, worldPosition);		// Replace with SetWorldPosition otherwise we can't parent the aiobject

			posDif = glm::vec2(targetWorldPosition.x - worldPosition.x, targetWorldPosition.z - worldPosition.z);

			if (glm::length2(posDif) < 3.0f)
				pathEnded = false;
		}

		// Should only be called when the enemy is moving like chasing, patrolling, etc
		if (game->GetTerrain())
		{
			float height = game->GetTerrain()->GetExactHeightAt(worldPosition.x, worldPosition.z);
			tm.SetLocalPosition(e, glm::vec3(worldPosition.x, height + 0.1f, worldPosition.z));
		}

		//float targetDistance = glm::length(tm.GetWorldPosition(target) - tm.GetWorldPosition(e));

		// Check if the enemy is close enough to the player to attack
		// Replace with entity
		/*if (targetDistance < attackRange && !attacking)
		{
			attacking = true;
			if (script)
				script->CallOnTargetInRange(target);
		}
		else if (targetDistance > attackRange && attacking)		// Move this to script!!!
		{
			AnimatedModel *am = GetAnimatedModel();
			if (am)
				am->PlayAnimation(0);
		}*/

		if (attacking)
		{
			attackTimer += dt;
			if (attackTimer > attackDelay)
			{
				attackTimer = 0.0f;
				attacking = false;
			}
		}

		if (turning)
		{
			glm::vec3 localRotEuler = glm::eulerAngles(tm.GetLocalRotation(e));

			float shortestAngle = utils::WrapAngle(localRotEuler.y, goalAngle);

			if (glm::abs(shortestAngle) < 0.1f)
			{
				turning = false;
				turnSpeed = 0.0f;
			}

			if (turning)
			{
				turnSpeed = glm::min(glm::abs(shortestAngle), 100.0f) * 4.0f;

				if (shortestAngle < 0.0f)
					tm.SetLocalRotationEuler(e, glm::vec3(localRotEuler.x, localRotEuler.y - dt * turnSpeed, localRotEuler.z));
				else
					tm.SetLocalRotationEuler(e, glm::vec3(localRotEuler.x, localRotEuler.y + dt * turnSpeed, localRotEuler.z));
			}
		}
	}

	void AIObject::SetTarget(Entity e)
	{
		// Check if entity is valid

		target = e;
		targetWorldPosition = game->GetTransformManager().GetWorldPosition(e);
	}

	void AIObject::SetState(int state)
	{
		if (state < 0 || state >= (int)AIState::AI_STATE_COUNT)
			this->state = AIState::IDLE;

		this->state = (AIState)state;

		if (this->state == AIState::IDLE)
			moveSpeed = 0.0f;
	}

	void AIObject::Serialize(Serializer &s)
	{
		s.Write(eyesOffset);
		s.Write(eyesRange);
		s.Write(attackRange);
		s.Write(attackDelay);
		s.Write(fov);
	}

	void AIObject::Deserialize(Serializer &s)
	{
		s.Read(eyesOffset);
		s.Read(eyesRange);
		s.Read(attackRange);
		s.Read(attackDelay);
		s.Read(fov);
	}

	void AIObject::TurnTo(float angle)
	{
		turning = true;
		goalAngle = angle;
	}
}
