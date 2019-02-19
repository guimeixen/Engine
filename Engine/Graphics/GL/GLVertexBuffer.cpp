#include "GLVertexBuffer.h"

#include "GLUtils.h"

namespace Engine
{
	GLVertexBuffer::GLVertexBuffer(Renderer *renderer, const void *data, unsigned int size, BufferUsage usage) : Buffer(BufferType::VertexBuffer)
	{
		this->size = size;
		this->usage = usage;

		glBindVertexArray(0);

		glGenBuffers(1, &id);
		glBindBuffer(GL_ARRAY_BUFFER, id);
		glBufferData(GL_ARRAY_BUFFER, size, data, glutils::UsageToGL(usage));
		glBindBuffer(GL_ARRAY_BUFFER, 0);
	}

	GLVertexBuffer::GLVertexBuffer(const void *data, unsigned int size, BufferUsage usage) : Buffer(BufferType::VertexBuffer)
	{
		this->size = size;
		this->usage = usage;

		glBindVertexArray(0);					// We always unbind the vao because when rendering we bind it but never unbind it and if we load something, like a model or terrain, at runtime there probably is
												// a vao bound and not unbinding it here would cause us to overwrite the vertex buffer binding of the vao

		glGenBuffers(1, &id);
		glBindBuffer(GL_ARRAY_BUFFER, id);
		glBufferData(GL_ARRAY_BUFFER, size, data, glutils::UsageToGL(usage));
		glBindBuffer(GL_ARRAY_BUFFER, 0);
	}

	GLVertexBuffer::~GLVertexBuffer()
	{
		if (id > 0)
			glDeleteBuffers(1, &id);
	}

	void GLVertexBuffer::BindTo(unsigned int bindingIndex)
	{
	}

	void GLVertexBuffer::Update(const void *data, unsigned int size, int offset)
	{
		//glBindBuffer(GL_ARRAY_BUFFER, id);
		//glBufferSubData(GL_ARRAY_BUFFER, offset, size, data);
		glNamedBufferSubData(id, offset, size, data);
	}
}
