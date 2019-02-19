#pragma once

#include "AStarNode.h"

#include <vector>

namespace Engine
{
	class AStarNodeHeap
	{
	public:
		AStarNodeHeap(int maxHeapSize);
		~AStarNodeHeap();

		void Add(AStarNode *item);
		AStarNode *RemoveFirst();
		void Update(AStarNode *item);
		bool Contains(const AStarNode *item);
		void Swap(AStarNode *item1, AStarNode *item2);
		void SortDown(AStarNode *item);
		void SortUp(AStarNode *item);

		size_t Size() const { return currentItemCount; }

	private:
		std::vector<AStarNode*> items;
		int currentItemCount;
	};
}
