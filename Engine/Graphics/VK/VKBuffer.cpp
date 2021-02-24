#include "VKBuffer.h"

#include "VKUtils.h"
#include "VKBase.h"
#include "Program/Log.h"
#include "Program/Utils.h"

#include <iostream>

namespace Engine
{
	VKBuffer::VKBuffer(VKBase *base, const void *data, unsigned int size, BufferType type, BufferUsage usage)
	{
		this->size = size;
		this->usage = usage;
		this->type = type;
		mapped = nullptr;	
		buffer = VK_NULL_HANDLE;
		stagingBuffer = VK_NULL_HANDLE;
		alloc = {};
		stagingAlloc = {};
		memProps = {};		
		vkUsage = 0;
		alignedSize = 0;
		device = base->GetDevice();		
		allocator = base->GetAllocator();

		VkPhysicalDevice physicalDevice = base->GetPhysicalDevice();

		if (type == BufferType::VertexBuffer)
		{
			if (usage == BufferUsage::STATIC)
			{
				VkBufferCreateInfo bufferInfo = {};
				bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
				bufferInfo.size = size;
				bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
				bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

				if (vkCreateBuffer(device, &bufferInfo, nullptr, &stagingBuffer) != VK_SUCCESS)
				{
					std::cout << "Error -> Failed to create vertex buffer!\n";
				}

				VkMemoryRequirements memReqs;
				vkGetBufferMemoryRequirements(device, stagingBuffer, &memReqs);

				allocator->Allocate(stagingAlloc, memReqs.size, vkutils::FindMemoryType(physicalDevice, memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT), false);

				vkBindBufferMemory(device, stagingBuffer, stagingAlloc.memory, stagingAlloc.offset);			// If offset non-zero then it's required to be divisible by memReqs.alignment

				void* mappedData;
				vkMapMemory(device, stagingAlloc.memory, stagingAlloc.offset, stagingAlloc.size, 0, &mappedData);
				memcpy(mappedData, data, size);
				vkUnmapMemory(device, stagingAlloc.memory);

				Create(physicalDevice, static_cast<VkDeviceSize>(size), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, false);
			}
			else if (usage == BufferUsage::DYNAMIC || usage == BufferUsage::STREAM)
			{
				if (data)
				{
					Create(physicalDevice, static_cast<VkDeviceSize>(size), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, false);

					Map();
					Update(data, size, 0);
					//Unmap();*/
				}
				else
				{
					Create(physicalDevice, static_cast<VkDeviceSize>(size), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, true);
					Map();
				}
			}
		}
		else if (type == BufferType::IndexBuffer)
		{
			if (usage == BufferUsage::STATIC)
			{
				VkBufferCreateInfo bufferInfo = {};
				bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
				bufferInfo.size = size;
				bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
				bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

				if (vkCreateBuffer(device, &bufferInfo, nullptr, &stagingBuffer) != VK_SUCCESS)
				{
					Log::Print(LogLevel::LEVEL_ERROR, "Error -> Failed to create staging index buffer!\n");
				}

				VkMemoryRequirements memReqs;
				vkGetBufferMemoryRequirements(device, stagingBuffer, &memReqs);

				allocator->Allocate(stagingAlloc, memReqs.size, vkutils::FindMemoryType(physicalDevice, memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT), false);

				vkBindBufferMemory(device, stagingBuffer, stagingAlloc.memory, stagingAlloc.offset);			// If offset non-zero then it's required to be divisible by memReqs.alignment

				void* mappedData;
				vkMapMemory(device, stagingAlloc.memory, stagingAlloc.offset, stagingAlloc.size, 0, &mappedData);
				memcpy(mappedData, data, size);
				vkUnmapMemory(device, stagingAlloc.memory);

				Create(physicalDevice, (VkDeviceSize)size, VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, false);
			}
			else if (usage == BufferUsage::DYNAMIC || usage == BufferUsage::STREAM)
			{
				if (data)
				{
					Create(physicalDevice, (VkDeviceSize)size, VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, false);

					Map();
					Update(data, size, 0);
					Unmap();
				}
				else
				{
					Create(physicalDevice, (VkDeviceSize)size, VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, false);
					Map();
				}
			}
		}
		else if (type == BufferType::UniformBuffer)
		{
			// Align the size to min ubo offset alignment so we can use offsets
			//unsigned int minUBOAlignment = (unsigned int)base->GetDeviceLimits().minUniformBufferOffsetAlignment;
			//alignedSize = utils::Align(size, minUBOAlignment);

			if (data)
			{
				Create(physicalDevice, static_cast<VkDeviceSize>(size), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, true);

				Map();
				Update(data, size, 0);
				Unmap();
			}
			else
			{
				Create(physicalDevice, static_cast<VkDeviceSize>(size), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, true);
				Map();
			}
		}
		else if (type == BufferType::StagingBuffer)
		{
			Create(physicalDevice, (VkDeviceSize)size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, false);
		}
		else if (type == BufferType::DrawIndirectBuffer)
		{
			Create(physicalDevice, (VkDeviceSize)size, VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, true);

			if (data)
			{
				Map();
				Update(data, size, 0);		// TODO: Use staging buffer for buffers that won't be updated on the cpu
				Unmap();
			}
		}
		else if (type == BufferType::ShaderStorageBuffer)
		{
			// IF DATA IS NOT NULL USE HOST_COHERENT. ADD STAGING BUFFER INSTEAD TO UPLOAD TO DEVICE_LOCAL
			if (usage == BufferUsage::DYNAMIC || data != nullptr)
			{
				Create(physicalDevice, (VkDeviceSize)size, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, true);
				Map();
				if (data)
				{
					Update(data, size, 0);		// TODO: Use staging buffer for buffers that won't be updated on the cpu
					Unmap();
				}
			}
			else if (usage == BufferUsage::STATIC)
			{
				Create(physicalDevice, (VkDeviceSize)size, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, true);
			}
		}
	}

	VKBuffer::~VKBuffer()
	{
		Dispose();
	}

	void VKBuffer::Create(VkPhysicalDevice physicalDevice, VkDeviceSize size, VkBufferUsageFlags usageFlags, VkMemoryPropertyFlags properties, bool exclusiveAlloc)
	{
		vkUsage = usageFlags;
		memProps = properties;

		VkBufferCreateInfo bufferInfo = {};
		bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferInfo.size = size;
		bufferInfo.usage = usageFlags;
		bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		if (vkCreateBuffer(device, &bufferInfo, nullptr, &buffer) != VK_SUCCESS)
		{
			Log::Print(LogLevel::LEVEL_ERROR, "Error -> Failed to create buffer!\n");
			return;
		}

		VkMemoryRequirements memReqs;
		vkGetBufferMemoryRequirements(device, buffer, &memReqs);

		//this->size = memReqs.size;
		alignedSize = memReqs.size;

		allocator->Allocate(alloc, memReqs.size, vkutils::FindMemoryType(physicalDevice, memReqs.memoryTypeBits, properties), exclusiveAlloc);

		// Associate memory with the buffer
		vkBindBufferMemory(device, buffer, alloc.memory, alloc.offset);			// If offset non-zero then it's required to be divisible by memReqs.alignment
	}

	void VKBuffer::Dispose()
	{
		if (buffer != VK_NULL_HANDLE)
		{
			vkDestroyBuffer(device, buffer, nullptr);
			allocator->Free(alloc);
		}
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
		// If we have HOST_COHERENT we don't need to manually flush, otherwise we do
		if ((memProps & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) != 0)
		{
			void *ptr = (char*)mapped + offset;
			memcpy(ptr, data, size);
		}
		else
		{
			void *ptr = (char*)mapped + offset;
			memcpy(ptr, data, size);

			VkMappedMemoryRange memoryRange = {};
			memoryRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
			memoryRange.memory = alloc.memory;
			memoryRange.size = VK_WHOLE_SIZE;
			memoryRange.offset = alloc.offset + static_cast<VkDeviceSize>(offset);

			vkFlushMappedMemoryRanges(device, 1, &memoryRange);

			// We could store the mapped memory range and then the renderer would flush all of them
		}	
	}

	void VKBuffer::Map()
	{
		if (!mapped)
			vkMapMemory(device, alloc.memory, alloc.offset, alloc.size, 0, &mapped);
	}

	void VKBuffer::Unmap()
	{
		if (mapped)
		{
			vkUnmapMemory(device, alloc.memory);
			mapped = nullptr;
		}
	}

	void VKBuffer::Flush(unsigned int offset)
	{
		VkMappedMemoryRange memoryRange = {};
		memoryRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
		memoryRange.memory = alloc.memory;
		memoryRange.size = VK_WHOLE_SIZE;
		memoryRange.offset = alloc.offset + static_cast<VkDeviceSize>(offset);

		vkFlushMappedMemoryRanges(device, 1, &memoryRange);
	}
}
