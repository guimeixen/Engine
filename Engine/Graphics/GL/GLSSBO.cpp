#include "GLSSBO.h"

#include "include\glew\glew.h"

namespace Engine
{
	GLSSBO::GLSSBO(const void *data, unsigned int size) : Buffer(BufferType::ShaderStorageBuffer)
	{
		//glGenBuffers(1, &id);
		glCreateBuffers(1, &id);
		/*glBindBuffer(GL_SHADER_STORAGE_BUFFER, id);
		glBufferData(GL_SHADER_STORAGE_BUFFER, size, data, GL_DYNAMIC_COPY);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);*/
		glNamedBufferStorage(id, size, data, GL_DYNAMIC_STORAGE_BIT);
	}

	GLSSBO::~GLSSBO()
	{
		if (id > 0)
			glDeleteBuffers(1, &id);
	}

	void GLSSBO::BindTo(unsigned int bindingIndex)
	{
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, bindingIndex, id);
	}

	void GLSSBO::Update(const void *data, unsigned int size, int offset)
	{
		glNamedBufferSubData(id, offset, size, data);
	}

}
