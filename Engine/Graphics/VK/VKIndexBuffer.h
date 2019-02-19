#pragma once

#include <vulkan\vulkan.h>

#include "Graphics\Buffers.h"
#include "VKAllocator.h"

namespace Engine
{
	class VKRenderer;
	class Renderer;
	class VKBase;

	class VKIndexBuffer : public Buffer
	{
	public:
		VKIndexBuffer();
		VKIndexBuffer(VKBase *context, const void *data, unsigned int size, BufferUsage usage);
		VKIndexBuffer(const void *data, unsigned int size, BufferUsage usage);
		~VKIndexBuffer();

		void Create(VkPhysicalDevice physicalDevice, VkDevice device, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties);
		void Dispose(VkDevice device);
		void DisposeStagingBuffer();

		void BindTo(unsigned int bindingIndex) override;
		void Update(const void *data, unsigned int size, int offset) override;

		void Map(VkDevice device);
		void Unmap(VkDevice device);

		VkBuffer GetBuffer() const { return buffer; }
		VkDeviceMemory GetDeviceMemory() const { return alloc.memory; }
		VkBuffer GetStagingBuffer() const { return stagingBuffer; }
		VkDeviceMemory GetStagingDeviceMemory() const { return stagingAlloc.memory; }
		void *Mapped() { return mapped; }
		VkDeviceSize GetSize() const { return size; }
		VkBufferUsageFlags GetUsage() const { return usage; }

	private:
		VKAllocator *allocator;
		VkDevice device;
		VkBuffer buffer;
		VkBuffer stagingBuffer;
		Allocation alloc;
		Allocation stagingAlloc;

		VkDeviceSize size;
		void *mapped;
		VkBufferUsageFlags usage;
	};
}
