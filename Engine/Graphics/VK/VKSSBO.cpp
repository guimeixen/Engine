#include "VKSSBO.h"

#include "VKBase.h"

#include <iostream>

namespace Engine
{
	VKSSBO::VKSSBO(const VKBase *context, const void *data, unsigned int size, BufferUsage usage) : Buffer(BufferType::ShaderStorageBuffer)
	{
		mapped = nullptr;
		buffer = VK_NULL_HANDLE;
		this->size = size;
		this->device = context->GetDevice();
		this->allocator = context->GetAllocator();

		// IF DATA IS NOT NULL USE HOST_COHERENT. ADD STAGING BUFFER INSTEAD TO UPLOAD TO DEVICE_LOCAL
		if (usage == BufferUsage::DYNAMIC || data != nullptr)
		{
			Create(context, static_cast<VkDeviceSize>(this->size), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
			Map();
			if (data)
			{
				Update(data, size, 0);		// TODO: Use staging buffer for buffers that won't be updated on the cpu
				Unmap();
			}
		}
		else if (usage == BufferUsage::STATIC)
		{
			Create(context, static_cast<VkDeviceSize>(this->size), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
		}
	}

	VKSSBO::~VKSSBO()
	{
		if (device != VK_NULL_HANDLE && buffer != VK_NULL_HANDLE)
		{
			vkDestroyBuffer(device, buffer, nullptr);
			allocator->Free(alloc);
		}
	}

	void VKSSBO::Create(const VKBase *context, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties)
	{
		this->usage = usage;

		VkBufferCreateInfo bufferInfo = {};
		bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferInfo.size = size;
		bufferInfo.usage = usage;
		bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;		// Buffers can be owned by a specific queue family or be shared between multiple at the same time.
																// This will only be used from the graphics queue, so we use exclusive access.

		if (vkCreateBuffer(device, &bufferInfo, nullptr, &buffer) != VK_SUCCESS)
		{
			std::cout << "Error -> Failed to create ssbo!\n";
		}

		// Buffer memory requirements
		VkMemoryRequirements memReqs;
		vkGetBufferMemoryRequirements(device, buffer, &memReqs);

		allocator->Allocate(alloc, memReqs.size, vkutils::FindMemoryType(context->GetPhysicalDevice(), memReqs.memoryTypeBits, properties), true);

		// Associate memory with the buffer
		vkBindBufferMemory(device, buffer, alloc.memory, 0);			// If offset non-zero then it's required to be divisible by memReqs.alignment
	}


	void VKSSBO::BindTo(unsigned int bindingIndex)
	{
	}

	void VKSSBO::Update(const void *data, unsigned int size, int offset)
	{
		void *ptr = (char*)mapped + offset;
		memcpy(ptr, data, size);

		VkMappedMemoryRange memoryRange = {};
		memoryRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
		memoryRange.memory = alloc.memory;
		//memoryRange.size = size;
		memoryRange.size = VK_WHOLE_SIZE;
		memoryRange.offset = 0;

		vkFlushMappedMemoryRanges(device, 1, &memoryRange);
	}

	void VKSSBO::Flush()
	{
		VkMappedMemoryRange memoryRange = {};
		memoryRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
		memoryRange.memory = alloc.memory;
		memoryRange.size = VK_WHOLE_SIZE;
		memoryRange.offset = 0;

		vkFlushMappedMemoryRanges(device, 1, &memoryRange);
	}

	void VKSSBO::Map()
	{
		if (!mapped)
			vkMapMemory(device, alloc.memory, 0, VK_WHOLE_SIZE, 0, &mapped);
	}

	void VKSSBO::Unmap()
	{
		if (mapped)
		{
			vkUnmapMemory(device, alloc.memory);
			mapped = nullptr;
		}
	}
}
