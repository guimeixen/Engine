#pragma once

#include "include\glew\glew.h"
#include "Graphics\Buffers.h"

namespace Engine
{
	class GLUniformBuffer : public Buffer
	{
	public:
		GLUniformBuffer(const void *data, unsigned int size);
		~GLUniformBuffer();

		void BindTo(unsigned int bindingIndex);
		void Update(const void *data, unsigned int size, int offset);
		unsigned int GetID() const { return id; }

	private:
		GLuint id;
	};
}
