#include "GXMUniformBuffer.h"

#include "GXMUtils.h"

namespace Engine
{
	GXMUniformBuffer::GXMUniformBuffer(const void *data, unsigned int size, BufferUsage usage) : Buffer(BufferType::UniformBuffer)
	{
		this->size = size;
		this->usage = usage;
		ubo = gxmutils::graphicsAlloc(SCE_KERNEL_MEMBLOCK_TYPE_USER_RW_UNCACHE, SCE_GXM_MEMORY_ATTRIB_RW, (size_t)size, &uboUID);
		if (data)
			memcpy(ubo, data, (size_t)size);
	}

	GXMUniformBuffer::~GXMUniformBuffer()
	{
		gxmutils::graphicsFree(uboUID);
	}

	void GXMUniformBuffer::BindTo(unsigned int bindingIndex)
	{
	}

	void GXMUniformBuffer::Update(const void *data, unsigned int size, int offset)
	{
		memcpy(ubo, data, size);
	}
}