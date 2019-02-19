#pragma once

#include "include\glew\glew.h"

#include "Graphics\Buffers.h"

namespace Engine
{
	class GLIndexBuffer : public Buffer
	{
	public:
		GLIndexBuffer(const void *data, unsigned int size, BufferUsage usage);
		~GLIndexBuffer();

		void BindTo(unsigned int bindingIndex) override;
		void Update(const void *data, unsigned int size, int offset) override;
		unsigned int GetID() const { return id; }

	private:
		GLuint id;
	};
}
