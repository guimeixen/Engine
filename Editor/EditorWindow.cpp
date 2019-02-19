#include "EditorWindow.h"

#include "imgui\imgui.h"
#include "imgui\imgui_dock.h"

EditorWindow::EditorWindow()
{
}

void EditorWindow::Init(Engine::Game *game, EditorManager *editorManager)
{
	this->game = game;
	this->editorManager = editorManager;
}

bool EditorWindow::BeginWindow(const char *name)
{
	return ImGui::BeginDock(name, &showWindow);
}

void EditorWindow::EndWindow()
{
	ImGui::EndDock();
}
