#pragma once

#include "VertexTypes.h"

#include <vector>

namespace Engine
{
	class Buffer;

	class VertexArray
	{
	public:
		virtual ~VertexArray();

		virtual void AddVertexBuffer(Buffer *vertexBuffer) = 0;

		Buffer *GetIndexBuffer() const;
		const std::vector<Buffer*> &GetVertexBuffers() const;
		const std::vector<VertexInputDesc> &GetVertexInputDescs() const { return vertexInputDescs; }

		void AddReference() { refCount++; }
		void RemoveReference() { if (refCount > 1) { refCount--; } else { delete this; } }

	protected:
		Buffer *indexBuffer = nullptr;
		std::vector<Buffer*> vertexBuffers;
		std::vector<VertexInputDesc> vertexInputDescs;
		unsigned int refCount = 0;
	};
}
