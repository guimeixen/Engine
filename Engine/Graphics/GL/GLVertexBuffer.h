#pragma once

#include "include\glew\glew.h"

#include "Graphics\Buffers.h"

namespace Engine
{
	class Renderer;

	class GLVertexBuffer : public Buffer
	{
	public:
		GLVertexBuffer(Renderer *renderer, const void *data, unsigned int size, BufferUsage usage);
		GLVertexBuffer(const void *data, unsigned int size, BufferUsage usage);
		~GLVertexBuffer();

		void BindTo(unsigned int bindingIndex) override;
		void Update(const void *data, unsigned int size, int offset) override;
		unsigned int GetID() const { return id; }

	private:
		GLuint id;
	};
}
