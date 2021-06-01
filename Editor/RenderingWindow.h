#pragma once

#include "EditorWindow.h"

#include <string>
#include <vector>

class RenderingWindow : public EditorWindow
{
public:
	void Init(Engine::Game *game, EditorManager *editorManager);
	void Render();

private:
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
	bool renderTerrainQuadTree = false;
};

