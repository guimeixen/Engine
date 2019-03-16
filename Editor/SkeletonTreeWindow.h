#pragma once

#include "EditorWindow.h"
#include "Graphics\Animation\AnimatedModel.h"

#include <vector>

class SkeletonTreeWindow : public EditorWindow
{
public:
	void Render();

private:
	void RenderSkeleton(Engine::Bone *bone);

private:
	bool hovered = false;
	int selectedEntityIndex = 0;
	Engine::Bone *selectedBone;
};

