#pragma once

#include "Graphics/Buffers.h"
#include "psp2/types.h"

namespace Engine
{
	class GXMVertexBuffer : public Buffer
	{
	public:
		GXMVertexBuffer(const void *data, unsigned int size, BufferUsage usage);
		~GXMVertexBuffer();

		void BindTo(unsigned int bindingIndex) override;
		void Update(const void *data, unsigned int size, int offset) override;

		void *GetVerticesHandle() const { return vertices; }

	private:
		SceUID verticesUID;
		void *vertices;
	};
}
