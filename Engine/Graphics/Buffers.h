#pragma once

#include "UniformBufferTypes.h"

namespace Engine
{
	enum class BufferType
	{
		VertexBuffer,
		IndexBuffer,
		UniformBuffer,
		DrawIndirectBuffer,
		ShaderStorageBuffer,
		StagingBuffer
	};

	class Buffer
	{
	public:
		Buffer() {}
		Buffer(BufferType type) { this->type = type; }
		virtual ~Buffer() {}

		virtual void BindTo(unsigned int bindingIndex) = 0;
		virtual void Update(const void *data, unsigned int size, int offset) = 0;

		void AddReference() { ++refCount; }
		void RemoveReference() { if (refCount > 1) { --refCount; } else { delete this; } }
		unsigned int GetRefCount() const { return refCount; }

		BufferType GetType() const { return type; }

	protected:
		unsigned int size = 0;
		BufferUsage usage = BufferUsage::STATIC;
		BufferType type;
	private:
		unsigned int refCount = 0;
	};
}

