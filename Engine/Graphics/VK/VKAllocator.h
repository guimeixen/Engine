#pragma once

#include <vulkan\vulkan.h>

#include <vector>

namespace Engine
{
	class VKBase;

	struct Allocation
	{
		VkDeviceMemory memory;
		VkDeviceSize offset;
		VkDeviceSize size;
		uint32_t memType;
		uint32_t blockIndex;
	};

	struct BlockLocation
	{
		uint32_t blockIndex;
		uint32_t freeSpotIndex;
	};

	struct FreeSpot
	{
		VkDeviceSize offset;
		VkDeviceSize size;
	};

	struct Block
	{
		Allocation alloc;
		std::vector<FreeSpot> freeSpots;	// Where in the block do we have free spot for allocation
		bool exclusive;						// Is this block exclusive for just one buffer/image. Otherwise if we map 2 objects at once for the same device memory we get an error. Ideal for UBO's
		VkDeviceSize totalAvailableSize;
	};

	struct MemoryPool
	{
		std::vector<Block> blocks;
	};

	struct MemoryInfo
	{
		VkDeviceSize totalAllocatedMemory;
		VkDeviceSize totalUsedMemory;
	};

	class VKAllocator
	{
	public:
		VKAllocator(VKBase *vkContext);
		~VKAllocator();

		// exclusiveAlloc -> Make an allocation exclusive for the objects requesting the allocation. It won't be shared with other objects
		void Allocate(Allocation &alloc, VkDeviceSize size, uint32_t memType, bool exclusiveAlloc);

		// Make sure to destroy the buffer/image when calling this function
		void Free(Allocation &alloc);

		void PrintStats();

		uint32_t GetCurrentAllocationsNum() const { return currentAllocations; }
		uint32_t GetTotalAllocationsMade() const { return totalAllocationsMade; }

	private:
		bool FindFreeBlock(BlockLocation &blockLocation, bool &isEmptyBlock, VkDeviceSize allocSize, uint32_t memTypeIndex, bool exclusiveAlloc);
		// Returns the index of the block in the memory pool with index memTypeIndex
		uint32_t AllocNewBlock(VkDeviceSize allocSize, uint32_t memTypeIndex, bool exclusiveAlloc);
		// Used to allocate a block that was previously freed
		void AllocEmptyBlock(uint32_t blockIndex, VkDeviceSize allocSize, uint32_t memTypeIndex, bool exclusiveAlloc);
		void MarkBlockChunkUsed(const BlockLocation &location, uint32_t memTypeIndex, VkDeviceSize allocSize);

	private:
		VkDevice device;
		uint32_t currentAllocations;
		uint32_t totalAllocationsMade;
		VkDeviceSize bufferImageGranularity;
		VkDeviceSize minBlockSize;

		std::vector<MemoryInfo> memInfo;
		std::vector<MemoryPool> memoryPools;		// We have a pool for each memory type
	};
}
