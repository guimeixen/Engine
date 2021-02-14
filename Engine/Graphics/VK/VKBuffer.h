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
		VKBuffer(VKBase *base, const void *data, unsigned int size, BufferType type, BufferUsage usage);
		~VKBuffer();
		
		void Dispose();
		void DisposeStagingBuffer();

		void BindTo(unsigned int bindingIndex) override;
		void Update(const void *data, unsigned int size, int offset) override;

		void Map();
		void Unmap();
		void Flush();

		VkBuffer GetBuffer() const { return buffer; }
		VkDeviceMemory GetDeviceMemory() const { return alloc.memory; }
		VkBuffer GetStagingBuffer() const { return stagingBuffer; }
		VkDeviceMemory GetStagingDeviceMemory() const { return stagingAlloc.memory; }
		void *Mapped() { return mapped; }
		VkDeviceSize GetSize() const { return size; }
		VkBufferUsageFlags GetUsage() const { return vkUsage; }

	private:
		void Create(VkPhysicalDevice physicalDevice, VkDeviceSize size, VkBufferUsageFlags usageFlags, VkMemoryPropertyFlags properties, bool exclusiveAlloc);

	private:
		VkDevice device;
		VKAllocator *allocator;
		VkBuffer buffer;
		VkBuffer stagingBuffer;
		
		Allocation alloc;
		Allocation stagingAlloc;	

		void *mapped;
		VkBufferUsageFlags vkUsage;
		VkMemoryPropertyFlags memProps;
	};
}
