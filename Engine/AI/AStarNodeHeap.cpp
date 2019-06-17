#include "AStarNodeHeap.h"

namespace Engine
{
	AStarNodeHeap::AStarNodeHeap(int maxHeapSize)
	{
		currentItemCount = 0;
		items.resize(maxHeapSize);
	}

	void AStarNodeHeap::Add(AStarNode *item)
	{
		if (currentItemCount == static_cast<int>(items.size()))
			return;
		item->heapIndex = currentItemCount;
		items[currentItemCount] = item;
		SortUp(item);
		currentItemCount++;
	}

	AStarNode *AStarNodeHeap::RemoveFirst()
	{
		AStarNode *firstItem = items[0];

		currentItemCount--;
		items[0] = items[currentItemCount];
		items[0]->heapIndex = 0;
		SortDown(items[0]);

		return firstItem;
	}

	void AStarNodeHeap::Update(AStarNode *item)
	{
		SortUp(item);
	}

	bool AStarNodeHeap::Contains(const AStarNode *item)
	{
		if (item->heapIndex < 0 || item->heapIndex >= (int)items.size())
			return false;

		return items[item->heapIndex] == item;
	}

	void AStarNodeHeap::Swap(AStarNode *item1, AStarNode *item2)
	{
		items[item1->heapIndex] = item2;
		items[item2->heapIndex] = item1;
		int item1Index = item1->heapIndex;
		item1->heapIndex = item2->heapIndex;
		item2->heapIndex = item1Index;
	}

	void AStarNodeHeap::SortDown(AStarNode *item)
	{
		while (true)
		{
			int childIndexLeft = item->heapIndex * 2 + 1;
			int childIndexRight = item->heapIndex * 2 + 2;
			int swapIndex = 0;

			if (childIndexLeft < currentItemCount)
			{
				swapIndex = childIndexLeft;

				if (childIndexRight < currentItemCount)
				{
					if (items[childIndexRight]->HasLowerFCost(items[childIndexLeft]))
					{
						swapIndex = childIndexRight;
					}
				}

				if (items[swapIndex]->HasLowerFCost(item))
				{
					Swap(item, items[swapIndex]);
				}
				else
					return;
			}
			else
				return;
		}
	}

	void AStarNodeHeap::SortUp(AStarNode *item)
	{
		int parentIndex = (item->heapIndex - 1) / 2;

		while (true)
		{
			AStarNode *parentItem = items[parentIndex];
			if (item->HasLowerFCost(parentItem))
			{
				Swap(item, parentItem);
			}
			else
			{
				break;
			}

			parentIndex = (item->heapIndex - 1) / 2;
		}
	}
}
