#pragma once

#include "Graphics\VertexArray.h"

namespace Engine
{
	class Buffer;

	class VKVertexArray : public VertexArray
	{
	public:
		VKVertexArray(const VertexInputDesc &desc, Buffer *vertexBuffer, Buffer *indexBuffer);
		VKVertexArray(const VertexInputDesc *descs, unsigned int descCount, const std::vector<Buffer*> &vertexBuffers, Buffer *indexBuffer);
		~VKVertexArray();

		void AddVertexBuffer(Buffer *vertexBuffer) override;
	};
}
