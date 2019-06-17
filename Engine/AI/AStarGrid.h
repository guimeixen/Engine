#pragma once

#include "AStarNodeHeap.h"

#include <forward_list>

namespace Engine
{
	class Game;
	class Renderer;

	class AStarGrid
	{
	public:
		AStarGrid();

		void Init(Game *game, const glm::vec2 &gridCenter, const glm::vec2 &gridSize, float nodeRadius);
		void Update();
		void Dispose();

		void RebuildGrid(Game *game, const glm::vec2 &newGridCenter);
		void RebuildImmediate(Game *game, const glm::vec2 &newGridCenter);
		void SetNeedsRebuild(bool needsRebuild);
		void SetGridCenter(const glm::vec2 &center);

		void PrepareDebugDraw();

		bool FindPath(const glm::vec2 &startPos, const glm::vec2 &targetPos, std::vector<glm::vec2> &nodeWaypoints, int maxSearch);

		void UpdateNode(const glm::vec2 &worldPos, bool walkable);
		AStarNode *NodeFromWorldPos(const glm::vec2 &pos);

		void SetNodesRebuildPerFrame(int count) { nodesRebuiltPerFrame = count; }

		const glm::vec2 &GetGridCenter() const { return gridCenter; }
		unsigned int GetNodesRebuiltPerFrame() const { return nodesRebuiltPerFrame; }

		void SaveGridToFile();
		void LoadGridFromFile();

	private:	
		std::forward_list<AStarNode*> GetNeighbours(AStarNode *node);
		int GetDistance(AStarNode *nodeA, AStarNode *nodeB);
		void RetracePath(AStarNode *startNode, AStarNode *targetNode, std::vector<glm::vec2> &nodeWaypoints);
		void SimplifyPath(std::vector<glm::vec2> &nodeWaypoints);

		void LoadDefaultGrid();

	private:
		bool isInit = false;
		Game *game;
		//std::vector<AStarNode*> grid;
		AStarNode *grid;
		unsigned int totalGridNodes;
		glm::vec2 gridSize;
		glm::vec2 gridCenter;
		glm::ivec2 gridCenterI;
		float nodeRadius;

		// To calculate how many nodes can fit in the given grid size
		float nodeDiameter;
		glm::ivec2 gridSizeXZ;	// how many nodes can fit

		int nodesRebuiltPerFrame = 2;
		bool isBuilt = true;
		int rebuildStartIndexX = 0;
		int rebuildStartIndexZ = 0;
		int rebuildStopIndexX = nodesRebuiltPerFrame;
		int rebuildStopIndexZ = 1;
		int gridIndex = 0;

		std::vector<AStarNode*> path;
	};
}
