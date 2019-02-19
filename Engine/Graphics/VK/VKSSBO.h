#pragma once

#include "Graphics\Buffers.h"
#include "VKAllocator.h"

namespace Engine
{
	class VKSSBO : public Buffer
	{
	public:
		VKSSBO(const VKBase *context, const void *data, unsigned int size, BufferUsage usage);
		~VKSSBO();

		void BindTo(unsigned int bindingIndex) override;
		void Update(const void *data, unsigned int size, int offset) override;
		void Flush();

		VkBuffer GetBuffer() const { return buffer; }
		void *GetMappedPtr() const { return mapped; }

	private:
		void Create(const VKBase *context, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties);
		void Map();
		void Unmap();

	private:
		VKAllocator *allocator;
		VkDevice device;
		VkBuffer buffer;
		Allocation alloc;

		void *mapped;
		VkBufferUsageFlags usage;
	};
}
