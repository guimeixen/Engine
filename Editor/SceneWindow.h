#pragma once

#include "Game\Game.h"
#include "Game\EntityManager.h"

#include "EditorNameManager.h"

#include "imgui\imgui.h"

#include <string>
#include <vector>

class EditorManager;
class Gizmo;

class SceneWindow
{
public:
	SceneWindow();
	~SceneWindow();

	void Init(Engine::Game *game, EditorManager *editorManager, Gizmo *gizmo);
	void Render();

	void Show(bool show);
	bool IsVisible() const { return showWindow; }

private:
	void RenderEntityName(const EditorName &name);

private:
	Engine::Game *game;
	EditorManager *editorManager;
	Engine::TransformManager *transformManager;
	Gizmo *gizmo;
	bool showWindow = true;
	
	bool hovered = false;
	int imguiID = 0;
	bool openPopup = false;
	bool popupOpen = false;
	ImVec2 dragDelta;
	char popupID[12];

	char objectName[128];
	bool shouldSelectEntity = false;
	Engine::Entity selectedEntity;
	Engine::Entity startEntity;
};
