#include "VKIndexBuffer.h"

//#include "VKRenderer.h"
#include "VKBase.h"

#include <iostream>

namespace Engine
{
	VKIndexBuffer::VKIndexBuffer() : Buffer(BufferType::IndexBuffer)
	{
		mapped = nullptr;
		//renderer = nullptr;
		size = 0;
	}

	VKIndexBuffer::VKIndexBuffer(VKBase *context, const void *data, unsigned int size, BufferUsage usage) : Buffer(BufferType::IndexBuffer)
	{
		AddReference();
		mapped = nullptr;
		this->size = size;
		//indexCount = size / sizeof(uint16_t);

		//this->renderer = static_cast<VKRenderer*>(renderer);

		VkDevice device = context->GetDevice();
		this->device = device;
		allocator = context->GetAllocator();

		VkPhysicalDevice physicalDevice = context->GetPhysicalDevice();

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

			/*VkMemoryAllocateInfo allocInfo = {};
			allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
			allocInfo.allocationSize = memReqs.size;
			allocInfo.memoryTypeIndex = vkutils::FindMemoryType(physicalDevice, memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

			if (vkAllocateMemory(device, &allocInfo, nullptr, &stagingMemory) != VK_SUCCESS)
			{
				std::cout << "Error -> Failed to allocate vertex buffer memory!\n";
			}*/

			// Associate memory with the buffer
			vkBindBufferMemory(device, stagingBuffer, stagingAlloc.memory, stagingAlloc.offset);			// If offset non-zero then it's required to be divisible by memReqs.alignment

			void *mappedData;
			vkMapMemory(device, stagingAlloc.memory, stagingAlloc.offset, stagingAlloc.size, 0, &mappedData);
			memcpy(mappedData, data, size);
			vkUnmapMemory(device, stagingAlloc.memory);		// Does not return a result as it can't fail

			Create(physicalDevice, device, static_cast<VkDeviceSize>(size), VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
		}
		else if (usage == BufferUsage::DYNAMIC || usage == BufferUsage::STREAM)
		{
			Create(physicalDevice, device, static_cast<VkDeviceSize>(size), VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

			if (data)
			{
				Map(device);
				Update(data, size, 0);
				Unmap(device);
			}
			else
			{
				Map(device);
			}
		}
	}

	VKIndexBuffer::VKIndexBuffer(const void *data, unsigned int size, BufferUsage usage) : Buffer(BufferType::IndexBuffer)
	{
		mapped = nullptr;
		this->size = size;
	}

	VKIndexBuffer::~VKIndexBuffer()
	{
		Dispose(device);
	}

	void VKIndexBuffer::Create(VkPhysicalDevice physicalDevice, VkDevice device, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties)
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

		this->size = memReqs.size;

		allocator->Allocate(alloc, memReqs.size, vkutils::FindMemoryType(physicalDevice, memReqs.memoryTypeBits, properties), false);

		/*VkMemoryAllocateInfo allocInfo = {};
		allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocInfo.allocationSize = memReqs.size;
		allocInfo.memoryTypeIndex = vkutils::FindMemoryType(physicalDevice, memReqs.memoryTypeBits, properties);

		if (vkAllocateMemory(device, &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS)
		{
			std::cout << "Error -> Failed to allocate vertex buffer memory!\n";
		}*/

		// Associate memory with the buffer
		vkBindBufferMemory(device, buffer, alloc.memory, alloc.offset);			// If offset non-zero then it's required to be divisible by memReqs.alignment
	}

	void VKIndexBuffer::Dispose(VkDevice device)
	{
		if (buffer != VK_NULL_HANDLE)
		{
			vkDestroyBuffer(device, buffer, nullptr);
			allocator->Free(alloc);
		}
	}

	void VKIndexBuffer::DisposeStagingBuffer()
	{
		if (device != VK_NULL_HANDLE && stagingBuffer != VK_NULL_HANDLE)
		{
			vkDestroyBuffer(device, stagingBuffer, nullptr);
			allocator->Free(stagingAlloc);
			stagingBuffer = VK_NULL_HANDLE;
		}
	}

	void VKIndexBuffer::BindTo(unsigned int bindingIndex)
	{
	}

	void VKIndexBuffer::Update(const void *data, unsigned int size, int offset)
	{// When memcpy the data into the mapped memory the driver may not copy the data into the buffer memory immediately
		// There are two ways to solve the problem:
		// 1- Use a memory heap that is host coherent
		// 2- Call vkFlsuhMappedMemoryRanges after writing to the mapped memory
		// Using the first option for now although the second is faster
		memcpy(mapped, data, size);
	}

	void VKIndexBuffer::Map(VkDevice device)
	{
		vkMapMemory(device, stagingAlloc.memory, stagingAlloc.offset, stagingAlloc.size, 0, &mapped);
	}

	void VKIndexBuffer::Unmap(VkDevice device)
	{
		if (mapped)
		{
			vkUnmapMemory(device, stagingAlloc.memory);		// Does not return a result as it can't fail
			mapped = nullptr;
		}
	}
}
