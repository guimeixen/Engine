#pragma once

#include "Graphics\Buffers.h"
#include "VKAllocator.h"

namespace Engine
{
	class Renderer;
	class VKBase;

	class VKBuffer : public Buffer
	{
	public:
		VKBuffer();
		VKBuffer(VKBase *context, const void *data, unsigned int size, BufferUsage usage);
		VKBuffer(const void *data, unsigned int size, BufferUsage usage);
		~VKBuffer();

		void Create(VKAllocator *allocator, VkPhysicalDevice physicalDevice, VkDevice device, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, bool exclusiveAlloc);
		void Dispose(VkDevice device);
		void DisposeStagingBuffer();

		void BindTo(unsigned int bindingIndex) override;
		void Update(const void *data, unsigned int size, int offset) override;

		void Map();
		void Unmap();

		VkBuffer GetBuffer() const { return buffer; }
		VkDeviceMemory GetDeviceMemory() const { return deviceAlloc.memory; }
		VkBuffer GetStagingBuffer() const { return stagingBuffer; }
		VkDeviceMemory GetStagingDeviceMemory() const { return stagingAlloc.memory; }
		void *Mapped() { return mapped; }
		VkDeviceSize GetSize() const { return size; }
		VkBufferUsageFlags GetUsage() const { return usage; }

	private:
		VkDevice device;
		VKAllocator *allocator;
		VkBuffer buffer;
		VkBuffer stagingBuffer;
		
		Allocation stagingAlloc;
		Allocation deviceAlloc;

		VkDeviceSize size;
		void *mapped;
		VkBufferUsageFlags usage;
		VkMemoryPropertyFlags memProps;
	};
}
