#pragma once

#include "Game\Game.h"

class EditorManager;

class EditorWindow
{
public:
	EditorWindow();
	virtual ~EditorWindow() {}

	virtual void Init(Engine::Game *game, EditorManager *editorManager);

	void Show(bool show) { showWindow = show; }
	bool IsVisible() const { return showWindow; }

protected:
	bool BeginWindow(const char *name);
	void EndWindow();

protected:
	Engine::Game *game = nullptr;
	EditorManager *editorManager = nullptr;
	bool showWindow = true;
};

