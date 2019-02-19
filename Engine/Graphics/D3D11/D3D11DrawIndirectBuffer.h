#pragma once

#include "Graphics/Buffers.h"

#include <d3d11_1.h>

namespace Engine
{
	class D3D11DrawIndirectBuffer : public Buffer
	{
	public:
		D3D11DrawIndirectBuffer(ID3D11Device *device, ID3D11DeviceContext *context, const void *data, unsigned int size);
		~D3D11DrawIndirectBuffer();

		void BindTo(unsigned int bindingIndex) override {}
		void Update(const void *data, unsigned int size, int offset) override {}

		ID3D11UnorderedAccessView *GetUAV() const { return uav; }

	private:
		ID3D11Buffer *buffer;
		ID3D11UnorderedAccessView *uav;
	};
}
