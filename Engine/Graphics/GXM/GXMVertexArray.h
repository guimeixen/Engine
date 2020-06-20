#pragma once

#include "Graphics\VertexArray.h"

namespace Engine
{
	class Buffer;

	class GXMVertexArray : public VertexArray
	{
	public:
		GXMVertexArray(const VertexInputDesc &desc, Buffer *vertexBuffer, Buffer *indexBuffer);
		GXMVertexArray(const VertexInputDesc *descs, unsigned int descCount, const std::vector<Buffer*> &vertexBuffers, Buffer *indexBuffer);
		~GXMVertexArray();

		void AddVertexBuffer(Buffer *vertexBuffer) override;
	};
}
