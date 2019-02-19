#include "GLDrawIndirectBuffer.h"

#include "include\glew\glew.h"

namespace Engine
{
	GLDrawIndirectBuffer::GLDrawIndirectBuffer(const void *data, unsigned int size) : Buffer(BufferType::DrawIndirectBuffer)
	{
		glGenBuffers(1, &id);
		glBindBuffer(GL_DRAW_INDIRECT_BUFFER, id);
		glBufferData(GL_DRAW_INDIRECT_BUFFER, size, data, GL_DYNAMIC_COPY);
		glBindBuffer(GL_DRAW_INDIRECT_BUFFER, 0);
	}

	GLDrawIndirectBuffer::~GLDrawIndirectBuffer()
	{
		if (id > 0)
			glDeleteBuffers(1, &id);
	}

	void GLDrawIndirectBuffer::BindTo(unsigned int bindingIndex)
	{
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, bindingIndex, id);		// Has to be bound as SSBO
	}

	void GLDrawIndirectBuffer::Update(const void *data, unsigned int size, int offset)
	{
		glBufferSubData(GL_DRAW_INDIRECT_BUFFER, offset, size, data);
	}
}
