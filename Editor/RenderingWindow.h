#pragma once

#include "Game\Game.h"

#include <string>

class EditorManager;

class RenderingWindow
{
public:
	void Init(Engine::Game *game, EditorManager *editorManager);
	void Render();

	void Show(bool show) { showWindow = show; }
	bool IsVisible() const { return showWindow; }

private:
	Engine::Game *game;
	EditorManager *editorManager;
	bool showWindow = true;

	bool showUI = true;

	std::vector<std::string> files;

	float nearPlane = 0.2f;
	float farPlane = 0.0f;
	float lightShaftsIntensity = 1.0f;
	std::vector<std::string> debugTypesStr;
	int selectedDebugType = 0;

	float worldTime = 0.0f;
	int month = 0;
	int day = 0;
	float azimuthOffset = 0.0f;
	bool enableGI = true;
	float voxelGridSize = 0.0f;

	float waterHeight = 0.0f;

	bool lockToTOD = true;
};

