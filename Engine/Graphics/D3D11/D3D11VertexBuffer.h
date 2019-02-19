#pragma once

#include "Graphics\Buffers.h"

#include <d3d11_1.h>

namespace Engine
{
	class D3D11VertexBuffer : public Buffer
	{
	public:
		D3D11VertexBuffer(ID3D11Device *device, ID3D11DeviceContext *context, const void *data, unsigned int size, BufferUsage usage);
		~D3D11VertexBuffer();

		void BindTo(unsigned int bindingIndex) override;
		void Update(const void *data, unsigned int size, int offset) override;

		ID3D11Buffer *GetBuffer() const { return buffer; }

	private:
		ID3D11DeviceContext *context;
		ID3D11Buffer *buffer;
	};
}
