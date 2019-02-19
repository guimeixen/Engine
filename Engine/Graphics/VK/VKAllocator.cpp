#include "VKAllocator.h"

#include "VKBase.h"
#include "Program\Log.h"

namespace Engine
{
	VKAllocator::VKAllocator(VKBase *vkContext)
	{
		device = vkContext->GetDevice();

		bufferImageGranularity = vkContext->GetDeviceLimits().bufferImageGranularity;
		Log::Print(LogLevel::LEVEL_INFO, "Buffer Image Granularity: %d\n", bufferImageGranularity);

		VkPhysicalDeviceMemoryProperties mp = vkContext->GetMemoryProperties();
		memInfo.resize(mp.memoryTypeCount);
		memoryPools.resize(mp.memoryTypeCount);

		minBlockSize = bufferImageGranularity * 20;

		currentAllocations = 0;
		totalAllocationsMade = 0;
	}

	VKAllocator::~VKAllocator()
	{
	}

	void VKAllocator::Allocate(Allocation &alloc, VkDeviceSize size, uint32_t memType, bool exclusive)
	{
		MemoryPool &pool = memoryPools[memType];

		// Allocation must be multiple of buffer image granularity
		VkDeviceSize realAllocSize = ((size / bufferImageGranularity) + 1) * bufferImageGranularity;    // eg 256 / 1024 = 0   +1 = 1   1*1024=1024

		memInfo[memType].totalUsedMemory += realAllocSize;

		BlockLocation location;
		bool isEmptyBlock = false;
		bool found = FindFreeBlock(location, isEmptyBlock, realAllocSize, memType, exclusive);

		if (!found)
		{
			location.blockIndex = AllocNewBlock(realAllocSize, memType, exclusive);
			location.freeSpotIndex = 0;		// it's the first allocation for the new block
		}
		else if (isEmptyBlock)
		{
			AllocEmptyBlock(location.blockIndex, realAllocSize, memType, exclusive);
			location.freeSpotIndex = 0;
		}

		Block &block = pool.blocks[location.blockIndex];
		if (block.exclusive == false && exclusive == true)		// Check so we don't set a exclusive block to false
			block.exclusive = true;

		alloc.memory = block.alloc.memory;
		alloc.offset = block.freeSpots[location.freeSpotIndex].offset;
		alloc.size = size;		// Size neeeds to be the unmodified size
		alloc.memType = memType;
		alloc.blockIndex = location.blockIndex;

		MarkBlockChunkUsed(location, memType, realAllocSize);
	}

	void VKAllocator::Free(Allocation &alloc)
	{
		if (alloc.size == 0)
			return;

		// Find this alloc in the blocks and mark it as free
		// if the block is empty free it
		VkDeviceSize realAllocSize = ((alloc.size / bufferImageGranularity) + 1) * bufferImageGranularity;

		MemoryPool &pool = memoryPools[alloc.memType];
		Block &block = pool.blocks[alloc.blockIndex];

		block.freeSpots.push_back({ alloc.offset, realAllocSize });		// Add a free spot/chunk for allocation where this allocation was
		block.totalAvailableSize += realAllocSize;

		memInfo[alloc.memType].totalUsedMemory -= realAllocSize;

		if (block.totalAvailableSize == block.alloc.size || block.exclusive)
		{
			// free the memory because there is nothing else using this block
			memInfo[alloc.memType].totalAllocatedMemory -= block.alloc.size;
			currentAllocations--;

			vkFreeMemory(device, block.alloc.memory, nullptr);

			// We can't erase the block because it would mess up the allocation block index
			// Erase the free spots to mark the block as empty
			block.freeSpots.clear();
			// Reset the block allocation
			block.alloc = {};
			block.totalAvailableSize = 0;

			Log::Print(LogLevel::LEVEL_INFO, "Free block found, freeing memory... Current allocs : %d\n", currentAllocations);
		}
	}

	void VKAllocator::PrintStats()
	{
		Log::Print(LogLevel::LEVEL_INFO, "\nCurrent allocations: %d\n", currentAllocations);
		Log::Print(LogLevel::LEVEL_INFO, "Total allocations: %d\n", totalAllocationsMade);
		for (size_t i = 0; i < memInfo.size(); i++)
		{
			if (memInfo[i].totalUsedMemory == 0)
				continue;

			Log::Print(LogLevel::LEVEL_INFO, "Mem type index: %d\n\tTotal in use: %.2f mib\n", i, memInfo[i].totalUsedMemory / 1024.0f / 1024.0f);
			Log::Print(LogLevel::LEVEL_INFO, "\tTotal Allocated: %.2f mib\n\n", memInfo[i].totalAllocatedMemory / 1024.0f / 1024.0f);
		}
	}

	bool VKAllocator::FindFreeBlock(BlockLocation &blockLocation, bool &isEmptyBlock, VkDeviceSize allocSize, uint32_t memTypeIndex, bool exclusiveAlloc)
	{
		MemoryPool &pool = memoryPools[memTypeIndex];

		for (size_t i = 0; i < pool.blocks.size(); i++)
		{
			const Block &block = pool.blocks[i];

			// If this block is reserved for just one allocation then ignore it
			if (block.exclusive)
				continue;

			// Check if the block is empty because it has been freed
			if (block.alloc.memory == VK_NULL_HANDLE)
			{
				isEmptyBlock = true;
				blockLocation.blockIndex = i;
				return true;
			}

			for (size_t j = 0; j < block.freeSpots.size(); j++)
			{
				// Check if the block is occupied or not
				bool canUseBlock = exclusiveAlloc ? block.freeSpots[j].offset == 0 : true;
				// Check if we have space inside this block for the allocation
				if (block.freeSpots[j].size >= allocSize && canUseBlock)
				{
					blockLocation.blockIndex = i;
					blockLocation.freeSpotIndex = j;
					return true;
				}
			}
		}

		return false;
	}

	uint32_t VKAllocator::AllocNewBlock(VkDeviceSize allocSize, uint32_t memTypeIndex, bool exclusiveAlloc)
	{
		VkDeviceSize newBlockSize = allocSize * 2;
		if (newBlockSize < minBlockSize && !exclusiveAlloc)
			newBlockSize = minBlockSize;

		Block newBlock = {};

		newBlock.alloc.memType = memTypeIndex;
		newBlock.alloc.size = newBlockSize;
		newBlock.totalAvailableSize = newBlockSize;

		VkMemoryAllocateInfo allocInfo = {};
		allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocInfo.allocationSize = newBlockSize;
		allocInfo.memoryTypeIndex = memTypeIndex;

		VkResult res = vkAllocateMemory(device, &allocInfo, nullptr, &newBlock.alloc.memory);

		if (res != VK_SUCCESS)
		{
			if (res == VK_ERROR_OUT_OF_DEVICE_MEMORY)
				Log::Print(LogLevel::LEVEL_ERROR, "Out of device memory!\n");
			else if (res == VK_ERROR_TOO_MANY_OBJECTS)
				Log::Print(LogLevel::LEVEL_ERROR, "Too many allocations!\n");
			else
				Log::Print(LogLevel::LEVEL_ERROR, "Error allocating memory!\n");

			return -1;
		}

		MemoryPool &pool = memoryPools[memTypeIndex];
		pool.blocks.push_back(newBlock);

		uint32_t newBlockIndex = pool.blocks.size() - 1;
		pool.blocks[newBlockIndex].freeSpots.push_back({ 0, newBlockSize });

		currentAllocations++;
		totalAllocationsMade++;

		memInfo[memTypeIndex].totalAllocatedMemory += newBlockSize;

		return newBlockIndex;
	}

	void VKAllocator::AllocEmptyBlock(uint32_t blockIndex, VkDeviceSize allocSize, uint32_t memTypeIndex, bool exclusiveAlloc)
	{
		VkDeviceSize newBlockSize = allocSize;
		if (!exclusiveAlloc)
		{
			newBlockSize *= 2;
			if (newBlockSize < minBlockSize)
				newBlockSize = minBlockSize;
		}

		MemoryPool &pool = memoryPools[memTypeIndex];
		Block &emptyBlock = pool.blocks[blockIndex];

		emptyBlock.alloc.memType = memTypeIndex;
		emptyBlock.alloc.size = newBlockSize;
		emptyBlock.totalAvailableSize = newBlockSize;

		VkMemoryAllocateInfo allocInfo = {};
		allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocInfo.allocationSize = newBlockSize;
		allocInfo.memoryTypeIndex = memTypeIndex;

		VkResult res = vkAllocateMemory(device, &allocInfo, nullptr, &emptyBlock.alloc.memory);

		if (res != VK_SUCCESS)
		{
			if (res == VK_ERROR_OUT_OF_DEVICE_MEMORY)
				Log::Print(LogLevel::LEVEL_ERROR, "Out of device memory!\n");
			else if (res == VK_ERROR_TOO_MANY_OBJECTS)
				Log::Print(LogLevel::LEVEL_ERROR, "Too many allocations!\n");
			else
				Log::Print(LogLevel::LEVEL_ERROR, "Error allocating memory!\n");

			return;
		}

		emptyBlock.freeSpots.push_back({ 0, newBlockSize });

		currentAllocations++;
		totalAllocationsMade++;

		memInfo[memTypeIndex].totalAllocatedMemory += newBlockSize;
	}

	void VKAllocator::MarkBlockChunkUsed(const BlockLocation &location, uint32_t memTypeIndex, VkDeviceSize allocSize)
	{
		MemoryPool &pool = memoryPools[memTypeIndex];
		Block &block = pool.blocks[location.blockIndex];

		FreeSpot &freeSpot = block.freeSpots[location.freeSpotIndex];

		block.totalAvailableSize -= allocSize;

		freeSpot.size -= allocSize;
		freeSpot.offset += allocSize;
	}
}
