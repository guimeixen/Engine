#pragma once

#include <vector>

#include "Game\Game.h"
#include "Graphics\Animation\AnimatedModel.h"

class EditorManager;

class SkeletonTreeWindow
{
public:
	SkeletonTreeWindow();

	void Init(Engine::Game *game, EditorManager *editorManager);
	void Render();

	void Show(bool show) { showWindow = show; }
	bool IsVisible() const { return showWindow; }

private:
	void RenderSkeleton(Engine::Bone *bone);

private:
	Engine::Game *game;
	EditorManager *editorManager;
	bool showWindow = true;
	bool hovered = false;
	int selectedEntityIndex = 0;
	Engine::Bone *selectedBone;
};

