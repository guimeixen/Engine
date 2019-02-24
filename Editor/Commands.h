#pragma once

#include "Game/EntityManager.h"
#include "Game/ComponentManagers/TransformManager.h"
#include "Graphics/Terrain/Terrain.h"

#include "include/glm/glm.hpp"

class EditorCommand
{
public:
	virtual ~EditorCommand() {}

	virtual void Execute() = 0;

	virtual void Undo() = 0;
	virtual void Redo() = 0;
};

class TransformCommand : public EditorCommand
{
public:
	TransformCommand(Engine::TransformManager *tm, Engine::Entity e, const glm::vec3 &newPosition, const glm::vec3 &newRotation, const glm::vec3 &newScale)
	{
		this->tm = tm;
		this->e = e;
		newPos = newPosition;
		newRot = newRotation;
		this->newScale = newScale;

		oldPos = tm->GetLocalPosition(e);
		oldRot = tm->GetLocalRotation(e);
		oldScale = tm->GetLocalScale(e);
	}

	void Execute() override
	{
		tm->SetLocalPosition(e, newPos);
		tm->SetLocalRotation(e, newRot);
		tm->SetLocalScale(e, newScale);
	}

	void Undo() override
	{
		tm->SetLocalPosition(e, oldPos);
		tm->SetLocalRotation(e, oldRot);
		tm->SetLocalScale(e, oldScale);
	}

	void Redo() override
	{
		tm->SetLocalPosition(e, newPos);
		tm->SetLocalRotation(e, newRot);
		tm->SetLocalScale(e, newScale);
	}

private:
	Engine::TransformManager *tm;
	Engine::Entity e;

	glm::vec3 newPos;
	glm::quat newRot;
	glm::vec3 newScale;

	glm::vec3 oldPos;
	glm::quat oldRot;
	glm::vec3 oldScale;
};


class VegetationPlacementCommand : public EditorCommand
{
public:
	VegetationPlacementCommand(Engine::Terrain *terrain, const std::vector<int> &ids, const glm::vec3 &rayOrigin, const glm::vec3 &rayDir)
	{
		this->terrain = terrain;
		this->ids = ids;

		terrain->PaintVegetation(ids, rayOrigin, rayDir);
	}

	void Execute() override
	{
	}

	void Undo() override
	{
		terrain->UndoVegetationPaint(ids);
	}

	void Redo() override
	{
		terrain->RedoVegetationPaint(ids);
	}

private:
	Engine::Terrain *terrain;
	std::vector<int> ids;
};