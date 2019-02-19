#pragma once

#include "Graphics\Buffers.h"

namespace Engine
{
	class GLDrawIndirectBuffer : public Buffer
	{
	public:
		GLDrawIndirectBuffer(const void *data, unsigned int size);
		~GLDrawIndirectBuffer();

		void BindTo(unsigned int bindingIndex) override;
		void Update(const void *data, unsigned int size, int offset) override;
		unsigned int GetID() const { return id; }

	private:
		unsigned int id;
	};
}
