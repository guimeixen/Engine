#include "VKVertexArray.h"

#include "VKBuffer.h"
#include "VKIndexBuffer.h"

namespace Engine
{
	VKVertexArray::VKVertexArray(const VertexInputDesc &desc, Buffer *vertexBuffer, Buffer *indexBuffer)
	{
		if (!vertexBuffer)
			return;

		if (indexBuffer)
		{
			if ((static_cast<VKIndexBuffer*>(indexBuffer)->GetUsage() & VK_BUFFER_USAGE_INDEX_BUFFER_BIT) == VK_BUFFER_USAGE_INDEX_BUFFER_BIT)
				this->indexBuffer = indexBuffer;
		}

		vertexBuffer->AddReference();
		vertexBuffers.push_back(vertexBuffer);

		vertexInputDescs.push_back(desc);
	}

	VKVertexArray::VKVertexArray(const VertexInputDesc *descs, unsigned int descCount, const std::vector<Buffer*> &vertexBuffers, Buffer *indexBuffer)
	{
		if (vertexBuffers.size() == 0 || !descs)
			return;

		if (indexBuffer)
		{
			if ((static_cast<VKIndexBuffer*>(indexBuffer)->GetUsage() & VK_BUFFER_USAGE_INDEX_BUFFER_BIT) == VK_BUFFER_USAGE_INDEX_BUFFER_BIT)
				this->indexBuffer = indexBuffer;
		}

		for (size_t i = 0; i < vertexBuffers.size(); i++)
		{
			vertexBuffers[i]->AddReference();
			this->vertexBuffers.push_back(vertexBuffers[i]);
		}

		for (unsigned int i = 0; i < descCount; i++)
		{
			vertexInputDescs.push_back(descs[i]);
		}
	}

	VKVertexArray::~VKVertexArray()
	{
	}

	void VKVertexArray::AddVertexBuffer(Buffer *vertexBuffer)
	{
		if (!vertexBuffer)
			return;

		vertexBuffer->AddReference();
		vertexBuffers.push_back(vertexBuffer);
	}
}
