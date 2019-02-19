#include "D3D11IndexBuffer.h"

#include <iostream>

namespace Engine
{
	D3D11IndexBuffer::D3D11IndexBuffer(ID3D11Device *device, const void *data, unsigned int size, BufferUsage usage) : Buffer(BufferType::IndexBuffer)
	{
		D3D11_BUFFER_DESC desc = {};
		desc.Usage = D3D11_USAGE_DEFAULT;
		desc.ByteWidth = size;
		desc.BindFlags = D3D11_BIND_INDEX_BUFFER;
		desc.CPUAccessFlags = 0;
		
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
			std::cout << "Failed to create index buffer\n";
		}
	}

	D3D11IndexBuffer::~D3D11IndexBuffer()
	{
		if (buffer)
			buffer->Release();
	}

	void D3D11IndexBuffer::BindTo(unsigned int bindingIndex)
	{
	}

	void D3D11IndexBuffer::Update(const void *data, unsigned int size, int offset)
	{
	}
}