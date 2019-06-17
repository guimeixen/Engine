#include "GXMVertexBuffer.h"

#include "GXMUtils.h"

namespace Engine
{
	GXMVertexBuffer::GXMVertexBuffer(const void *data, unsigned int size, BufferUsage usage) : Buffer(BufferType::VertexBuffer)
	{
		this->size = size;
		this->usage = usage;
		if (usage == BufferUsage::STATIC)
		{
			vertices = gxmutils::graphicsAlloc(SCE_KERNEL_MEMBLOCK_TYPE_USER_RW_UNCACHE, SCE_GXM_MEMORY_ATTRIB_READ, (size_t)size, &verticesUID);
		}
		else if (usage == BufferUsage::DYNAMIC)
		{
			vertices = gxmutils::graphicsAlloc(SCE_KERNEL_MEMBLOCK_TYPE_USER_RW_UNCACHE, SCE_GXM_MEMORY_ATTRIB_RW, (size_t)size, &verticesUID);
		}
		
		if (data)
			memcpy(vertices, data, (size_t)size);
	}

	GXMVertexBuffer::~GXMVertexBuffer()
	{
		gxmutils::graphicsFree(verticesUID);
	}

	void GXMVertexBuffer::BindTo(unsigned int bindingIndex)
	{

	}

	void GXMVertexBuffer::Update(const void *data, unsigned int size, int offset)
	{
		memcpy(vertices, data, (size_t)size);
	}
}
