#pragma once

#include "Graphics/Buffers.h"
#include "psp2/types.h"

namespace Engine
{
	class GXMUniformBuffer : public Buffer
	{
	public:
		GXMUniformBuffer(const void *data, unsigned int size, BufferUsage usage);
		~GXMUniformBuffer();

		void BindTo(unsigned int bindingIndex) override;
		void Update(const void *data, unsigned int size, int offset) override;

		void *GetUBO() const { return ubo; }

	private:
		SceUID uboUID;
		void *ubo;
	};
}
