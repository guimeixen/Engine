#pragma once

#include "Game\Game.h"

class Gizmo
{
public:
	Gizmo();
	~Gizmo();

	void Init(Engine::Game *game);
	void Update(float dt);
	void PrepareRender();
	void SetSelectedEntity(Engine::Entity e);
	void DeselectEntity() { selected = false; }
	void SetRayPoint(const glm::vec2 &p) { rayPoint = p; }
	void CanRaycast(bool b);

	Engine::Entity GetSelectedEntity() const { return selectedEntity; }
	bool WasEntitySelected() const { return wasEntitySelected; }

private:
	Engine::Game *game;
	Engine::Entity selectedEntity;
	Engine::TransformManager *transformManager;
	Engine::RigidBody *selectedRigidBody;
	Engine::Collider *selectedCollider;
	Engine::Trigger *selectedTrigger;
	bool selected;
	Engine::AABB xBB;
	Engine::AABB yBB;
	Engine::AABB zBB;
	glm::vec2 rayPoint;
	bool xIntersected;
	bool yIntersected;
	bool zIntersected;
	bool moving;

	bool canRaycast;
	bool wasEntitySelected = false;

	Engine::Buffer *matUBO;

	glm::vec3 gizmoPos;

	glm::vec3 lastIntersectPoint;
	glm::vec3 intersectPoint;
};

