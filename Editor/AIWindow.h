#pragma once

#include "Game\Game.h"

class EditorManager;

class AIWindow
{
public:
	AIWindow();
	~AIWindow();

	void Init(Engine::Game *game, EditorManager *editorManager);
	void Update();
	void Render();

	void Show(bool show) { showWindow = show; }
	bool IsVisible() const { return showWindow; }

private:
	Engine::Game *game;
	EditorManager *editorManager;
	bool showWindow = true;

	glm::vec2 gridCenter;
	int nodesRebuiltPerFrame = 0;
	bool selectGrid = false;
	bool walkable = false;
	glm::vec3 intersectionPoint = glm::vec3(0.0f);
	bool intersected = false;
};

