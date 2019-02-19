#pragma once

#include "AStarGrid.h"

#include <queue>

namespace Engine
{
	class Game;
	class Renderer;

	class AISystem
	{
	public:
		AISystem();
		~AISystem();

		void Init(Game *game);
		void Update();
		void Dispose();

		bool RequestPath(const glm::vec3 &startPos, const glm::vec3 &endPos, std::vector<glm::vec2> &nodeWaypoints, int maxSearch = 99999);

		void PrepareDebugDraw();

		AStarGrid &GetAStarGrid() { return aStarGrid; }

		void SetShowGrid(bool showGrid) { this->showGrid = showGrid; }
		bool GetShowGrid() const { return showGrid; }

	private:
		AStarGrid aStarGrid;
		bool showGrid;
		//std::queue<> pathRequests;
	};
}
