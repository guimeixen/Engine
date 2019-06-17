#include "GXMIndexBuffer.h"

#include "GXMUtils.h"

namespace Engine
{
	GXMIndexBuffer::GXMIndexBuffer(const void *data, unsigned int size, BufferUsage usage) : Buffer(BufferType::IndexBuffer)
	{
		this->size = size;
		this->usage = usage;
		indices = (unsigned short*)gxmutils::graphicsAlloc(SCE_KERNEL_MEMBLOCK_TYPE_USER_RW_UNCACHE, SCE_GXM_MEMORY_ATTRIB_READ, (size_t)size, &indicesUID);
		memcpy(indices, data, (size_t)size);
	}

	GXMIndexBuffer::~GXMIndexBuffer()
	{
		gxmutils::graphicsFree(indicesUID);
	}

	void GXMIndexBuffer::BindTo(unsigned int bindingIndex)
	{
	}

	void GXMIndexBuffer::Update(const void *data, unsigned int size, int offset)
	{
	}
}
