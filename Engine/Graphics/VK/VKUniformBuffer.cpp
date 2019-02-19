#include "VKUniformBuffer.h"

#include "VKRenderer.h"

#include <iostream>

namespace Engine
{
	VKUniformBuffer::VKUniformBuffer(const VKBase *context, const void *data, unsigned int size) : Buffer(BufferType::UniformBuffer)
	{
		mapped = nullptr;
		buffer = VK_NULL_HANDLE;
		this->size = size;
		this->device = context->GetDevice();
		this->allocator = context->GetAllocator();

		if (data)
		{
			Create(context, static_cast<VkDeviceSize>(size), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

			Map(device);
			Update(data, size, 0);
			Unmap(device);
		}
		else
		{
			Create(context, static_cast<VkDeviceSize>(this->size), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
			Map(device);
		}
	}

	VKUniformBuffer::~VKUniformBuffer()
	{
		if (device)
			Dispose(device);
	}

	void VKUniformBuffer::Create(const VKBase *context, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties)
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
			std::cout << "Error -> Failed to create vertex buffer!\n";
		}

		// Buffer memory requirements
		VkMemoryRequirements memReqs;
		vkGetBufferMemoryRequirements(device, buffer, &memReqs);

		allocator->Allocate(alloc, memReqs.size, vkutils::FindMemoryType(context->GetPhysicalDevice(), memReqs.memoryTypeBits, properties), true);

		// Associate memory with the buffer
		vkBindBufferMemory(device, buffer, alloc.memory, 0);			// If offset non-zero then it's required to be divisible by memReqs.alignment
	}

	void VKUniformBuffer::Dispose(VkDevice device)
	{
		if (buffer != VK_NULL_HANDLE)
		{
			vkDestroyBuffer(device, buffer, nullptr);
			allocator->Free(alloc);
		}
	}

	void VKUniformBuffer::Update(const void *data, unsigned int size, int offset)
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

	void VKUniformBuffer::Map(VkDevice device)
	{
		if (!mapped)
			vkMapMemory(device, alloc.memory, 0, VK_WHOLE_SIZE, 0, &mapped);
	}

	void VKUniformBuffer::Unmap(VkDevice device)
	{
		if (mapped)
		{
			vkUnmapMemory(device, alloc.memory);		// Does not return a result as it can't fail
			mapped = nullptr;
		}
	}
}
