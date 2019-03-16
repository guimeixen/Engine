#pragma once

#include "EditorWindow.h"
#include "Graphics\Terrain\Terrain.h"

#include "include\glm\glm.hpp"

#include <vector>

class TerrainWindow : public EditorWindow
{
public:
	TerrainWindow();

	void Init(Engine::Game *game, EditorManager *editorManager);
	void Render();

	void Paint(const glm::vec2 &mousePos);

	bool IsSelected() const { return isSelected; }

private:
	void FindFilesInDirectory(const std::string &dir, const char *extension);		// It's necessary to clear the vector before calling the function otherwise the values from the previous find will remain
	void AddNewObject();
	void AddModelLOD1(Engine::Vegetation &v);
	void AddModelLOD2(Engine::Vegetation &v);

	void HandleVegModel(const Engine::Vegetation &v);

private:
	Engine::Terrain *currentTerrain;
	bool isSelected = false;
	std::vector<std::string> files;

	float heightScale = 1.0f;
	float brushRadius = 1.0f;
	std::string resStr;

	std::vector<bool> selection;
	bool selected = false;
	int id = -1;
	bool popupOpen = false;
	bool choosingLOD1 = false;
	bool choosingLOD2 = false;

	bool castShadows = 0;

	std::string texturesPopupNames[6];

	std::vector<int> ids;
};

