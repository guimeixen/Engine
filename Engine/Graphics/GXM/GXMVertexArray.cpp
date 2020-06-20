#include "GXMVertexArray.h"

#include "Graphics/Buffers.h"

namespace Engine
{
	GXMVertexArray::GXMVertexArray(const VertexInputDesc &desc, Buffer *vertexBuffer, Buffer *indexBuffer)
	{
		if (!vertexBuffer)
			return;

		if (indexBuffer)
		{
			this->indexBuffer = indexBuffer;
		}

		vertexBuffer->AddReference();
		vertexBuffers.push_back(vertexBuffer);		// Limit vertex buffers? To avoid the dynamic memory allocation

		vertexInputDescs.push_back(desc);
	}

	GXMVertexArray::GXMVertexArray(const VertexInputDesc *descs, unsigned int descCount, const std::vector<Buffer*> &vertexBuffers, Buffer *indexBuffer)
	{
		if (vertexBuffers.size() == 0 || !descs)
			return;

		if (indexBuffer)
		{
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

	GXMVertexArray::~GXMVertexArray()
	{
	}

	void GXMVertexArray::AddVertexBuffer(Buffer *vertexBuffer)
	{
		if (!vertexBuffer)
			return;

		vertexBuffer->AddReference();
		vertexBuffers.push_back(vertexBuffer);
	}
}
