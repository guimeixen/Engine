#include "GLUniformBuffer.h"

#include "GLUtils.h"

namespace Engine
{
	GLUniformBuffer::GLUniformBuffer(const void *data, unsigned int size) : Buffer(BufferType::UniformBuffer)
	{
		this->size = size;

		glGenBuffers(1, &id);
		glBindBuffer(GL_UNIFORM_BUFFER, id);
		glBufferData(GL_UNIFORM_BUFFER, size, data, GL_DYNAMIC_DRAW);
		glBindBuffer(GL_UNIFORM_BUFFER, 0);
	}

	GLUniformBuffer::~GLUniformBuffer()
	{
		if (id > 0)
			glDeleteBuffers(1, &id);
	}

	void GLUniformBuffer::BindTo(unsigned int bindingIndex)
	{
		glBindBufferBase(GL_UNIFORM_BUFFER, bindingIndex, id);
	}

	void GLUniformBuffer::Update(const void *data, unsigned int size, int offset)
	{
		//glBindBuffer(GL_UNIFORM_BUFFER, id);
		//glBufferSubData(GL_UNIFORM_BUFFER, offset, size, data);
		glNamedBufferSubData(id, offset, size, data);
	}
}
