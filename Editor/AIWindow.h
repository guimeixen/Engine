#pragma once

#include "EditorWindow.h"

#include "include/glm/glm.hpp"

class AIWindow : public EditorWindow
{
public:
	AIWindow();

	void Update();
	void Render();

private:
	glm::vec2 gridCenter;
	int nodesRebuiltPerFrame = 0;
	bool selectGrid = false;
	bool walkable = false;
	glm::vec3 intersectionPoint = glm::vec3(0.0f);
	bool intersected = false;
};

