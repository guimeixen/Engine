#include "D3D11VertexBuffer.h"

#include <iostream>

namespace Engine
{
	D3D11VertexBuffer::D3D11VertexBuffer(ID3D11Device *device, ID3D11DeviceContext *context, const void *data, unsigned int size, BufferUsage usage) : Buffer(BufferType::VertexBuffer)
	{
		this->context = context;

		D3D11_BUFFER_DESC desc = {};
		if (usage == BufferUsage::STATIC)
		{
			desc.Usage = D3D11_USAGE_DEFAULT;
			desc.CPUAccessFlags = 0;
		}
		else if (usage == BufferUsage::DYNAMIC)
		{
			desc.Usage = D3D11_USAGE_DYNAMIC;
			desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		}

		desc.ByteWidth = size;
		desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

		HRESULT hr;
		if (data)		// Check if we have data initially because otherwise initData must be null
		{
			D3D11_SUBRESOURCE_DATA initData = {};
			initData.pSysMem = data;

			hr = device->CreateBuffer(&desc, &initData, &buffer);
		}
		else
		{
			hr = device->CreateBuffer(&desc, nullptr, &buffer);
		}

		if (FAILED(hr))
		{
			std::cout << "Failed to create vertex buffer\n";
		}
	}

	D3D11VertexBuffer::~D3D11VertexBuffer()
	{
		if (buffer)
			buffer->Release();
	}

	void D3D11VertexBuffer::BindTo(unsigned int bindingIndex)
	{
	}

	void D3D11VertexBuffer::Update(const void *data, unsigned int size, int offset)
	{
		assert(context);
		D3D11_MAPPED_SUBRESOURCE mappedResource = {};
		context->Map(buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
		void *mapped = (char*)mappedResource.pData + offset;
		memcpy(mapped, data, size);
		context->Unmap(buffer, 0);
	}
}
