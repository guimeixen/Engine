#pragma once

#include "include\glm\glm.hpp"

namespace Engine
{
	struct AStarNode
	{
		AStarNode *parent;		// Used to trace back the path to the starting position
		glm::vec2 worldPos;
		glm::ivec2 gridPos;
		bool walkable;
		//bool isStatic;			// Is set to true when loading the grid for the first time and the node is an obstacle. If it is true then walkable will always remain false even when a dynamic object tries to update the node as non-walkable
		int gCost;				// Distance from the starting node
		int hCost;				// (heuristic or manhattan) distance from the end node

		int heapIndex;

		int fCost() const { return gCost + hCost; }

		// Returns true if this node has lower fCost than other node. If fCost are the same it checks the hCost. Returns false if the fCost is higher or the hCost is equal or higher
		bool HasLowerFCost(const AStarNode *other)
		{
			if (fCost() < other->fCost())
				return true;
			else if (fCost() == other->fCost())
				return hCost < other->hCost;

			return false;
		}
	};
}
