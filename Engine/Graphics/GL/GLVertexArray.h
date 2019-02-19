#pragma once

#include "Graphics\VertexArray.h"

namespace Engine
{
	class Buffer;

	class GLVertexArray : public VertexArray
	{
	public:
		GLVertexArray(const VertexInputDesc &desc, Buffer *vertexBuffer, Buffer *indexBuffer);
		GLVertexArray(const VertexInputDesc *descs, unsigned int descCount, const std::vector<Buffer*> &vertexBuffers, Buffer *indexBuffer);
		~GLVertexArray();

		void AddVertexBuffer(Buffer *vertexBuffer) override;

		unsigned int GetID() const { return id; }

	private:
		unsigned int id;
		unsigned int lastAttribLocation;
		unsigned int nextInputDesc;
	};
}
