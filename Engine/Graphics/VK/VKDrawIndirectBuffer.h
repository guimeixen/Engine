#pragma once

#include "Graphics/Buffers.h"
#include "VKAllocator.h"

namespace Engine
{
	class VKBase;

	class VKDrawIndirectBuffer : public Buffer
	{
	public:
		VKDrawIndirectBuffer(const VKBase *context, const void *data, unsigned int size);
		~VKDrawIndirectBuffer();

		void BindTo(unsigned int bindingIndex) override;
		void Update(const void *data, unsigned int size, int offset) override;

		VkBuffer GetBuffer() const { return buffer; }

	private:
		void Create(const VKBase *context, VkDeviceSize size, VkBufferUsageFlags usageFlags, VkMemoryPropertyFlags properties);
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
