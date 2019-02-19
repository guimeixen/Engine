#pragma once

#include "Graphics\Buffers.h"

#include <d3d11_1.h>

namespace Engine
{
	class D3D11UniformBuffer : public Buffer
	{
	public:
		D3D11UniformBuffer(ID3D11Device *device, ID3D11DeviceContext *context, const void *data, unsigned int size, BufferUsage usage);
		~D3D11UniformBuffer();

		void BindTo(unsigned int bindingIndex) override {}
		void Update(const void *data, unsigned int size, int offset) override;

		void Map();
		void UpdateMapped(const void *data, unsigned int size, int offset);
		void Unmap();

		ID3D11Buffer *GetBuffer() const { return contantBuffer; }

	private:
		ID3D11Buffer *contantBuffer;
		ID3D11DeviceContext *context;
		D3D11_MAPPED_SUBRESOURCE mappedResource = {};
	};
}
