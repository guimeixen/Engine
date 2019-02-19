#include "VKBuffer.h"

#include "VKUtils.h"
//#include "VKRenderer.h"
#include "VKBase.h"

#include <iostream>

namespace Engine
{
	VKBuffer::VKBuffer() : Buffer(BufferType::VertexBuffer)
	{
		device = VK_NULL_HANDLE;
		mapped = nullptr;
		size = 0;
		buffer = VK_NULL_HANDLE;
		stagingBuffer = VK_NULL_HANDLE;
		deviceAlloc = {};
		stagingAlloc = {};
	}

	VKBuffer::VKBuffer(VKBase *context, const void *data, unsigned int size, BufferUsage usage) : Buffer(BufferType::VertexBuffer)
	{
		mapped = nullptr;
		this->size = size;
		buffer = VK_NULL_HANDLE;
		stagingBuffer = VK_NULL_HANDLE;
		deviceAlloc = {};
		stagingAlloc = {};

		VkDevice device = context->GetDevice();
		this->device = device;
		VkPhysicalDevice physicalDevice = context->GetPhysicalDevice();
		allocator = context->GetAllocator();

		if (usage == BufferUsage::STATIC)
		{
			VkBufferCreateInfo bufferInfo = {};
			bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
			bufferInfo.size = size;
			bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
			bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;		// Buffers can be owned by a specific queue family or be shared between multiple at the same time.
																	// This will only be used from the graphics queue, so we use exclusive access.

			if (vkCreateBuffer(device, &bufferInfo, nullptr, &stagingBuffer) != VK_SUCCESS)
			{
				std::cout << "Error -> Failed to create vertex buffer!\n";
			}

			// Buffer memory requirements
			VkMemoryRequirements memReqs;
			vkGetBufferMemoryRequirements(device, stagingBuffer, &memReqs);

			this->size = memReqs.size;

			allocator->Allocate(stagingAlloc, memReqs.size, vkutils::FindMemoryType(physicalDevice, memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT), false);

			// Associate memory with the buffer
			vkBindBufferMemory(device, stagingBuffer, stagingAlloc.memory, stagingAlloc.offset);			// If offset non-zero then it's required to be divisible by memReqs.alignment

			void *mappedData;
			vkMapMemory(device, stagingAlloc.memory, stagingAlloc.offset, stagingAlloc.size, 0, &mappedData);
			memcpy(mappedData, data, size);
			vkUnmapMemory(device, stagingAlloc.memory);		// Does not return a result as it can't fail


			Create(allocator, physicalDevice, device, static_cast<VkDeviceSize>(size), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, false);
		}
		else if (usage == BufferUsage::DYNAMIC || usage == BufferUsage::STREAM)
		{
			if (data)
			{
				Create(allocator, physicalDevice, device, static_cast<VkDeviceSize>(size), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, false);

				Map();
				Update(data, size, 0);
				//Unmap(device);*/
			}
			else
			{
				Create(allocator, physicalDevice, device, static_cast<VkDeviceSize>(size), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, true);
				Map();
			}
		}
	}

	VKBuffer::VKBuffer(const void *data, unsigned int size, BufferUsage usage) : Buffer(BufferType::VertexBuffer)
	{
		mapped = nullptr;
		this->size = size;
	}

	VKBuffer::~VKBuffer()
	{
		Dispose(device);
	}

	void VKBuffer::Create(VKAllocator *allocator, VkPhysicalDevice physicalDevice, VkDevice device, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, bool exclusiveAlloc)
	{
		this->usage = usage;
		this->device = device;
		this->allocator = allocator;
		memProps = properties;

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

		this->size = memReqs.size;

		allocator->Allocate(deviceAlloc, memReqs.size, vkutils::FindMemoryType(physicalDevice, memReqs.memoryTypeBits, properties), exclusiveAlloc);

		// Associate memory with the buffer
		vkBindBufferMemory(device, buffer, deviceAlloc.memory, deviceAlloc.offset);			// If offset non-zero then it's required to be divisible by memReqs.alignment
	}

	void VKBuffer::Dispose(VkDevice device)
	{
		if (device != VK_NULL_HANDLE)
		{
			vkDestroyBuffer(device, buffer, nullptr);
			allocator->Free(deviceAlloc);
		}
		device = VK_NULL_HANDLE;
	}

	void VKBuffer::DisposeStagingBuffer()
	{
		if (device != VK_NULL_HANDLE && stagingBuffer != VK_NULL_HANDLE)
		{
			vkDestroyBuffer(device, stagingBuffer, nullptr);
			allocator->Free(stagingAlloc);
			stagingBuffer = VK_NULL_HANDLE;
		}
	}

	void VKBuffer::BindTo(unsigned int bindingIndex)
	{
	}

	void VKBuffer::Update(const void *data, unsigned int size, int offset)
	{
		if ((memProps & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) == VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)
		{
			void *ptr = (char*)mapped + offset;
			memcpy(ptr, data, size);
		}
		else if ((memProps & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) == VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)
		{
			void *ptr = (char*)mapped + offset;
			memcpy(ptr, data, size);

			VkMappedMemoryRange memoryRange = {};
			memoryRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
			memoryRange.memory = deviceAlloc.memory;
			//memoryRange.size = size;
			memoryRange.size = VK_WHOLE_SIZE;
			memoryRange.offset = deviceAlloc.offset + static_cast<VkDeviceSize>(offset);
			vkFlushMappedMemoryRanges(device, 1, &memoryRange);
		}	
	}

	void VKBuffer::Map()
	{
		vkMapMemory(device, deviceAlloc.memory, deviceAlloc.offset, deviceAlloc.size, 0, &mapped);
	}

	void VKBuffer::Unmap()
	{
		if (mapped)
		{
			vkUnmapMemory(device, deviceAlloc.memory);		// Does not return a result as it can't fail
			mapped = nullptr;
		}
	}
}
