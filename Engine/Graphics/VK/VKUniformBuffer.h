#pragma once

#include "Graphics\Buffers.h"
#include "VKAllocator.h"

#include <vulkan\vulkan.h>

namespace Engine
{
	class Renderer;
	class VKRenderer;
	class VKBase;
	class VKAllocator;

	class VKUniformBuffer : public Buffer
	{
	public:
		VKUniformBuffer(const VKBase *context, const void *data, unsigned int size);
		~VKUniformBuffer();

		void Update(const void *data, unsigned int size, int offset) override;
		void BindTo(unsigned int bindingIndex) override { this->bindingIndex = bindingIndex; }

		void Map(VkDevice device);
		void Unmap(VkDevice device);

		VkBuffer GetBuffer() const { return buffer; }
		VkDeviceMemory GetDeviceMemory() const { return alloc.memory; }
		void *Mapped() { return mapped; }
		unsigned int GetSize() const { return size; }
		VkBufferUsageFlags GetUsage() const { return usage; }
		unsigned int GetBindingIndex() const { return bindingIndex; }

	private:
		void Create(const VKBase *context, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties);
		void Dispose(VkDevice device);

	private:
		VKAllocator *allocator;
		VkDevice device;
		VkBuffer buffer;
		Allocation alloc;

		void *mapped;
		VkBufferUsageFlags usage;

		unsigned int bindingIndex;
	};
}
