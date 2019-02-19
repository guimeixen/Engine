#pragma once

#include "Graphics/Buffers.h"

#include <d3d11_1.h>

namespace Engine
{
	class D3D11SSBO : public Buffer
	{
	public:
		D3D11SSBO(ID3D11Device *device, ID3D11DeviceContext *context, const void *data, unsigned int size, unsigned int stride, BufferUsage usage);
		~D3D11SSBO();

		void BindTo(unsigned int bindingIndex) override;
		void Update(const void *data, unsigned int size, int offset) override;

		ID3D11Buffer *GetBuffer() const { return buffer; }
		ID3D11ShaderResourceView *GetSRV() const { return srv; }
		ID3D11UnorderedAccessView *GetUAV() const { return uav; }
		void *GetMappedPtr() const { return mappedResource.pData; }

		void Map();
		void Unmap();

	private:
		ID3D11DeviceContext *context;
		ID3D11Buffer *buffer;
		ID3D11ShaderResourceView *srv;
		ID3D11UnorderedAccessView *uav;
		D3D11_MAPPED_SUBRESOURCE mappedResource = {};
	};
}

