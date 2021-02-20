#pragma once

#include "EditorWindow.h"
#include "EditorNameManager.h"

#include "imgui/imgui.h"

#include <string>

namespace Engine
{
	class TransformManager;
}

class Gizmo;

class SceneWindow : public EditorWindow
{
public:
	SceneWindow();

	void Init(Engine::Game *game, EditorManager *editorManager);
	void Render();

private:
	void RenderEntityName(const EditorName &name);

private:
	Engine::TransformManager *transformManager;
	Gizmo *gizmo;
	
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
