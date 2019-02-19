#include "VertexArray.h"

#include "Graphics\Renderer.h"
#include "Graphics\GL\GLVertexArray.h"
#include "Graphics\Buffers.h"
#include "Program\Utils.h"
#include "VK\VKVertexArray.h"

namespace Engine
{
	VertexArray::~VertexArray()
	{
		if (indexBuffer)
			indexBuffer->RemoveReference();

		for (size_t i = 0; i < vertexBuffers.size(); i++)
		{
			vertexBuffers[i]->RemoveReference();
		}
	}

	Buffer *VertexArray::GetIndexBuffer() const
	{
		return indexBuffer;
	}

	const std::vector<Buffer*> &VertexArray::GetVertexBuffers() const
	{
		return vertexBuffers;
	}
}
