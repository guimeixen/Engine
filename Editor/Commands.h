#pragma once

#include "Game\Object.h"
#include "Graphics\Terrain\Terrain.h"

#include "include\glm\glm.hpp"

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
	TransformCommand(Engine::Object *obj, const glm::vec3 &newPosition, const glm::vec3 &newRotation, const glm::vec3 &newScale)
	{
		this->obj = obj;
		newPos = newPosition;
		newRot = newRotation;
		this->newScale = newScale;

		oldPos = obj->GetLocalPosition();
		oldRot = obj->GetLocalRotationEuler();
		oldScale = obj->GetLocalScale();
	}

	void Execute() override
	{
		obj->SetLocalPosition(newPos);
		obj->SetLocalRotationEuler(newRot);
		obj->SetLocalScale(newScale);
	}

	void Undo() override
	{
		obj->SetLocalPosition(oldPos);
		obj->SetLocalRotationEuler(oldRot);
		obj->SetLocalScale(oldScale);
	}

	void Redo() override
	{

	}

private:
	Engine::Object *obj;

	glm::vec3 newPos;
	glm::vec3 newRot;
	glm::vec3 newScale;

	glm::vec3 oldPos;
	glm::vec3 oldRot;
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