#include "GLIndexBuffer.h"

#include "GLUtils.h"

namespace Engine
{
	GLIndexBuffer::GLIndexBuffer(const void *data, unsigned int size, BufferUsage usage) : Buffer(BufferType::IndexBuffer)
	{
		this->size = size;
		this->usage = usage;

		glBindVertexArray(0);					// We always unbind the vao because when rendering we bind it but never unbind it and if we load something, like a model or terrain, at runtime there probably is
												// a vao bound and not unbinding it here would cause us to overwrite the index buffer binding of the vao

		glGenBuffers(1, &id);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, id);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, size, data, glutils::UsageToGL(usage));
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	}

	GLIndexBuffer::~GLIndexBuffer()
	{
		if (id > 0)
			glDeleteBuffers(1, &id);
	}

	void GLIndexBuffer::BindTo(unsigned int bindingIndex)
	{
	}

	void GLIndexBuffer::Update(const void *data, unsigned int size, int offset)
	{
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, id);
		glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, offset, size, data);
	}
}
