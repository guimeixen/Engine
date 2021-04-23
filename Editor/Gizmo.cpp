#include "Gizmo.h"

#include "Graphics\Effects\DebugDrawManager.h"

#include "Program\Input.h"
#include "Program\Utils.h"
#include "Program\Log.h"
#include "Physics\RigidBody.h"
#include "Physics\Collider.h"
#include "Physics\Trigger.h"

#include "include\glm\gtc\matrix_transform.hpp"
#include "include\glm\gtc\type_ptr.hpp"

#include <iostream>

Gizmo::Gizmo()
{
	selected = false;
	xIntersected = false;
	yIntersected = false;
	zIntersected = false;
	moving = false;

	selectedRigidBody = nullptr;
	selectedCollider = nullptr;
	selectedTrigger = nullptr;

	canRaycast = false;
}

Gizmo::~Gizmo()
{
}

void Gizmo::Init(Engine::Game *game)
{
	this->game = game;
	transformManager = &game->GetTransformManager();
}

void Gizmo::Update(float dt)
{
	if (game->IsPlaying())
		selected = false;

	wasEntitySelected = false;

	// Object focus
	if (selected && Engine::Input::WasKeyPressed(Engine::Keys::KEY_F) && !game->IsPlaying())
	{
		Engine::FPSCamera *cam = game->GetMainCamera();
		glm::vec3 center;
		if (game->GetModelManager().HasModel(selectedEntity))
		{
			const Engine::AABB &aabb = game->GetModelManager().GetAABB(selectedEntity);
			center = (aabb.min + aabb.max) / 2.0f;
		}
		else
		{
			center = transformManager->GetLocalToWorld(selectedEntity)[3];
		}
		
		cam->SetPosition(center - cam->GetFront() * 4.0f);
	}

	if (Engine::Input::WasMouseButtonReleased(Engine::MouseButtonType::Left) && canRaycast && !game->IsPlaying())
	{
		if(!xIntersected && !yIntersected && !zIntersected)
		{
			selectedRigidBody = nullptr;
			selectedCollider = nullptr;
			selectedTrigger = nullptr;
			
			bool intersected = game->PerformRayIntersection(rayPoint, selectedEntity);
			if (intersected)
			{
				wasEntitySelected = true;
				selected = true;

				Engine::PhysicsManager &pm = game->GetPhysicsManager();

				if (pm.HasRigidBody(selectedEntity))
					selectedRigidBody = pm.GetRigidBody(selectedEntity);
				if (pm.HasCollider(selectedEntity))
					selectedCollider = pm.GetCollider(selectedEntity);
				if (pm.HasTrigger(selectedEntity))
					selectedTrigger = pm.GetTrigger(selectedEntity);
			}
			else
			{
				selected = false;
				selectedEntity = { std::numeric_limits<unsigned int>::max() };
			}
		}
	}

	if (selected)
	{
		glm::vec3 pos = transformManager->GetLocalToWorld(selectedEntity)[3];

		float dist = glm::length(game->GetEditorCamera().GetPosition() - pos);
		glm::vec3 scale = glm::vec3(dist * 0.4f);

		xBB.min = glm::vec3(pos.x, pos.y - 0.05f, pos.z - 0.05f);
		xBB.max = glm::vec3(pos.x + scale.x, pos.y + 0.05f, pos.z + 0.05f);

		yBB.min = glm::vec3(pos.x - 0.05f, pos.y, pos.z - 0.05f);
		yBB.max = glm::vec3(pos.x + 0.05f, pos.y + scale.y, pos.z + 0.05f);

		zBB.min = glm::vec3(pos.x - 0.05f, pos.y - 0.05f, pos.z);
		zBB.max = glm::vec3(pos.x + 0.05f, pos.y + 0.05f, pos.z + scale.z);

		if (Engine::Input::IsMouseButtonDown(Engine::MouseButtonType::Left) && canRaycast)
		{
			glm::vec3 dir = Engine::utils::GetRayDirection(rayPoint, &game->GetEditorCamera());
			glm::vec3 camPos = game->GetEditorCamera().GetPosition();

			// Test the x axis first and then only proceed to the others if we didn't intersect it
			if (!moving && Engine::utils::RayAABBIntersection(camPos, dir, xBB))
			{
				xIntersected = true;
				yIntersected = zIntersected = false;
			}
			else if (!moving && Engine::utils::RayAABBIntersection(camPos, dir, yBB))
			{
				yIntersected = true;
				xIntersected = zIntersected = false;
			}
			else if (!moving && Engine::utils::RayAABBIntersection(camPos, dir, zBB))
			{
				zIntersected = true;
				xIntersected = yIntersected = false;
			}

			if (xIntersected || yIntersected || zIntersected)
			{
				lastIntersectPoint = intersectPoint;
				intersectPoint = dir * dist + camPos;

				if (!moving)
					lastIntersectPoint = intersectPoint;

				glm::vec3 delta = intersectPoint - lastIntersectPoint;

				if (xIntersected)
				{
					moving = true;
					pos.x += delta.x;
				}
				else if (yIntersected)
				{
					moving = true;
					pos.y += delta.y;
				}
				else if (zIntersected)
				{
					moving = true;
					pos.z += delta.z;
				}

				if (transformManager->HasParent(selectedEntity))
				{
					glm::mat4 m = glm::translate(glm::mat4(1.0f), pos);
					Engine::Entity e = transformManager->GetParent(selectedEntity);
					glm::mat4 invLocalToWorld = glm::inverse(transformManager->GetLocalToWorld(e));
					m = invLocalToWorld * m;															// Transform the world pos from the gizmo to local pos
					transformManager->SetLocalPosition(selectedEntity, m[3]);
				}
				else
				{
					transformManager->SetLocalPosition(selectedEntity, pos);
				}

				glm::mat4 localToWorld = transformManager->GetLocalToWorld(selectedEntity);
				// Normalize scale
				localToWorld[0] = glm::normalize(localToWorld[0]);
				localToWorld[1] = glm::normalize(localToWorld[1]);
				localToWorld[2] = glm::normalize(localToWorld[2]);

				if (selectedTrigger)
					selectedTrigger->SetTransform(localToWorld);

				if (selectedCollider)
					selectedCollider->SetTransform(localToWorld);

				if (selectedRigidBody)
					selectedRigidBody->SetPosition(localToWorld[3]);

				// Also update all child triggers, colliders, rbs
				if (transformManager->HasChildren(selectedEntity))
				{
					Engine::Entity child = transformManager->GetFirstChild(selectedEntity);

					glm::mat4 localToWorld;
					Engine::PhysicsManager &physicsManager = game->GetPhysicsManager();

					while (child.IsValid())
					{
						localToWorld = transformManager->GetLocalToWorld(child);
						// Normalize scale
						localToWorld[0] = glm::normalize(localToWorld[0]);
						localToWorld[1] = glm::normalize(localToWorld[1]);
						localToWorld[2] = glm::normalize(localToWorld[2]);

						Engine::Trigger* tr = physicsManager.GetTriggerSafe(child);
						if (tr)
							tr->SetTransform(localToWorld);

						Engine::Collider* col = physicsManager.GetColliderSafe(child);
						if (col)
							col->SetTransform(localToWorld);

						Engine::RigidBody* rb = physicsManager.GetRigidBodySafe(child);
						if (rb)
							rb->SetPosition(localToWorld[3]);

						child = transformManager->GetNextSibling(child);
					}
				}

				wasEntitySelected = true;
			}
		}
	}

	if (Engine::Input::IsMouseButtonDown(Engine::MouseButtonType::Left) == false)
	{
		moving = false;
		xIntersected = false;
		yIntersected = false;
		zIntersected = false;
	}
}

void Gizmo::PrepareRender()
{
	if (selected)
	{	
		glm::mat4 xAxis = glm::translate(glm::mat4(1.0f), (xBB.min + xBB.max) / 2.0f);
		xAxis = glm::scale(xAxis, xBB.max - xBB.min);

		glm::mat4 yAxis = glm::translate(glm::mat4(1.0f), (yBB.min + yBB.max) / 2.0f);
		yAxis = glm::scale(yAxis, yBB.max - yBB.min);

		glm::mat4 zAxis = glm::translate(glm::mat4(1.0f), (zBB.min + zBB.max) / 2.0f);
		zAxis = glm::scale(zAxis, zBB.max - zBB.min);

		game->GetDebugDrawManager()->AddTranslationGizmo(xAxis, yAxis, zAxis);
	}
}

void Gizmo::SetSelectedEntity(Engine::Entity e)
{
	selectedEntity = e;
	selected = true;

	selectedRigidBody = nullptr;
	selectedCollider = nullptr;
	selectedTrigger = nullptr;

	Engine::PhysicsManager &pm = game->GetPhysicsManager();

	if (pm.HasRigidBody(selectedEntity))
		selectedRigidBody = pm.GetRigidBody(selectedEntity);
	if (pm.HasCollider(selectedEntity))
		selectedCollider = pm.GetCollider(selectedEntity);
	if (pm.HasTrigger(selectedEntity))
		selectedTrigger = pm.GetTrigger(selectedEntity);
}

void Gizmo::CanRaycast(bool b)
{
	canRaycast = b;
}
