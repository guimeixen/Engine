#pragma once

#include "Graphics/Buffers.h"
#include "psp2/types.h"

namespace Engine
{
	class GXMIndexBuffer : public Buffer
	{
	public:
		GXMIndexBuffer(const void *data, unsigned int size, BufferUsage usage);
		~GXMIndexBuffer();

		void BindTo(unsigned int bindingIndex) override;
		void Update(const void *data, unsigned int size, int offset) override;

		unsigned short *GetIndicesHandle() const { return indices; }

	private:
		SceUID indicesUID;
		unsigned short *indices;
	};
}
