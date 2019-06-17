#include "AISystem.h"

#include "Program/Log.h"

#include <chrono>
#include <iostream>

namespace Engine
{

	AISystem::AISystem()
	{
		showGrid = false;
	}

	void AISystem::Init(Game *game)
	{
		///aStarGrid.Init(game, aStarGrid.GetGridCenter(), glm::vec2(380.0f, 450.0f), 0.5f);

		/*std::chrono::high_resolution_clock::time_point t1 = std::chrono::high_resolution_clock::now();
		std::vector<glm::vec2> nodeWaypoints;
		bool pathfound = aStarGrid.FindPath(glm::vec2(0.0f, 0.0f), glm::vec2(511.0f, 511.0f), &nodeWaypoints);
		std::chrono::high_resolution_clock::time_point t2 = std::chrono::high_resolution_clock::now();

		auto duration = std::chrono::duration_cast<std::chrono::microseconds>(t2 - t1).count();

		std::cout << "\nPath found: " << pathfound << '\n';
		std::cout << "Path time: " << duration << " us\n";
		std::cout << "Path time: " << duration / 1000 << " ms\n\n";*/

		Log::Print(LogLevel::LEVEL_INFO, "Init AI System\n");
	}

	void AISystem::Update()
	{
		///aStarGrid.Update();
	}

	void AISystem::Dispose()
	{
		///aStarGrid.Dispose();

		Log::Print(LogLevel::LEVEL_INFO, "Disposing AI system\n");
	}

	bool AISystem::RequestPath(const glm::vec3 &startPos, const glm::vec3 &endPos, std::vector<glm::vec2> &nodeWaypoints, int maxSearch)
	{
		///return aStarGrid.FindPath(glm::vec2(startPos.x, startPos.z), glm::vec2(endPos.x, endPos.z), nodeWaypoints, maxSearch);
		return false;
	}

	void AISystem::PrepareDebugDraw()
	{
		///if (showGrid)
		///	aStarGrid.PrepareDebugDraw();
	}
}
