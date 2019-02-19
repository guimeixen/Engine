#include "GLVertexArray.h"

#include "include\glew\glew.h"

#include "GLIndexBuffer.h"
#include "GLVertexBuffer.h"

namespace Engine
{
	GLVertexArray::GLVertexArray(const VertexInputDesc &desc, Buffer *vertexBuffer, Buffer *indexBuffer)
	{
		if (!vertexBuffer || desc.attribs.size() == 0)
			return;

		glGenVertexArrays(1, &id);
		glBindVertexArray(id);

		if (indexBuffer)
		{
			this->indexBuffer = indexBuffer;
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, static_cast<GLIndexBuffer*>(indexBuffer)->GetID());
		}

		if (vertexBuffer)
		{
			glBindBuffer(GL_ARRAY_BUFFER, static_cast<const GLVertexBuffer*>(vertexBuffer)->GetID());
			vertexBuffer->AddReference();
			vertexBuffers.push_back(vertexBuffer);
		}

		unsigned int attribLocation = 0;

		for (size_t i = 0; i < desc.attribs.size(); i++)
		{
			const VertexAttribute &v = desc.attribs[i];

			if (v.vertexAttribFormat == VertexAttributeFormat::FLOAT)
				glVertexAttribPointer(attribLocation, v.count, GL_FLOAT, GL_FALSE, desc.stride, (GLvoid*)v.offset);
			else if (v.vertexAttribFormat == VertexAttributeFormat::INT)
				glVertexAttribIPointer(attribLocation, v.count, GL_INT, desc.stride, (GLvoid*)v.offset);

			if (desc.instanced)
			{
				glVertexAttribDivisor(attribLocation, 1);
			}

			glEnableVertexAttribArray(attribLocation);

			attribLocation++;
		}

		nextInputDesc = 1;
		lastAttribLocation = attribLocation;

		glBindVertexArray(0);

		vertexInputDescs.push_back(desc);
	}

	GLVertexArray::GLVertexArray(const VertexInputDesc *descs, unsigned int descCount, const std::vector<Buffer*> &vertexBuffers, Buffer *indexBuffer)
	{
		if (!descs)
			return;

		glGenVertexArrays(1, &id);
		glBindVertexArray(id);

		if (indexBuffer)
		{
			this->indexBuffer = indexBuffer;
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, static_cast<GLIndexBuffer*>(indexBuffer)->GetID());
		}

		unsigned int attribLocation = 0;
		nextInputDesc = 0;

		// We set the vertex input desc for the vertex buffers we have available. If there are less vertex buffers than inputs descs then when calling AddVertexBuffer() it will use the next one available
		for (size_t i = 0; i < vertexBuffers.size(); i++)
		{
			glBindBuffer(GL_ARRAY_BUFFER, static_cast<GLVertexBuffer*>(vertexBuffers[i])->GetID());				// The descs should be in the order you want to use the vertex buffers

			for (unsigned int j = 0; j < descs[i].attribs.size(); j++)
			{
				const VertexAttribute &v = descs[i].attribs[j];

				if (v.vertexAttribFormat == VertexAttributeFormat::FLOAT)
					glVertexAttribPointer(attribLocation, v.count, GL_FLOAT, GL_FALSE, descs[i].stride, (GLvoid*)v.offset);
				else if (v.vertexAttribFormat == VertexAttributeFormat::INT)
					glVertexAttribIPointer(attribLocation, v.count, GL_INT, descs[i].stride, (GLvoid*)v.offset);

				if (descs[i].instanced)
				{
					glVertexAttribDivisor(attribLocation, 1);
				}

				glEnableVertexAttribArray(attribLocation);

				attribLocation++;
			}
			lastAttribLocation++;

			vertexBuffers[i]->AddReference();
			this->vertexBuffers.push_back(vertexBuffers[i]);

			nextInputDesc++;
		}

		lastAttribLocation = attribLocation;

		glBindVertexArray(0);

		for (unsigned int i = 0; i < descCount; i++)
		{
			vertexInputDescs.push_back(descs[i]);
		}
	}

	GLVertexArray::~GLVertexArray()
	{
		if (id > 0)
			glDeleteVertexArrays(1, &id);
	}

	void GLVertexArray::AddVertexBuffer(Buffer *vertexBuffer)
	{
		if (!vertexBuffer)
			return;

		glBindVertexArray(id);

		unsigned int attribLocation = lastAttribLocation;

		glBindBuffer(GL_ARRAY_BUFFER, static_cast<GLVertexBuffer*>(vertexBuffer)->GetID());

		const VertexInputDesc &desc = vertexInputDescs[nextInputDesc];
		for (size_t i = 0; i < desc.attribs.size(); i++)
		{
			const VertexAttribute &v = desc.attribs[i];

			if (v.vertexAttribFormat == VertexAttributeFormat::FLOAT)
				glVertexAttribPointer(attribLocation, v.count, GL_FLOAT, GL_FALSE, desc.stride, (GLvoid*)v.offset);
			else if (v.vertexAttribFormat == VertexAttributeFormat::INT)
				glVertexAttribIPointer(attribLocation, v.count, GL_INT, desc.stride, (GLvoid*)v.offset);

			if (desc.instanced)
			{
				glVertexAttribDivisor(attribLocation, 1);			// There's no need for more than one instance
			}

			glEnableVertexAttribArray(attribLocation);

			attribLocation++;
		}

		lastAttribLocation = attribLocation;
		nextInputDesc++;

		vertexBuffer->AddReference();
		vertexBuffers.push_back(vertexBuffer);

		glBindVertexArray(0);
	}
}
