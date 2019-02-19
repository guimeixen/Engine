#include "D3D11UniformBuffer.h"

#include <iostream>

namespace Engine
{
	D3D11UniformBuffer::D3D11UniformBuffer(ID3D11Device *device, ID3D11DeviceContext *context, const void *data, unsigned int size, BufferUsage usage) : Buffer(BufferType::UniformBuffer)
	{
		this->context = context;
		mappedResource = {};

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
		desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;

		HRESULT hr;
		if (data)		// Check if we have data initially because otherwise initData must be null
		{
			D3D11_SUBRESOURCE_DATA initData = {};
			initData.pSysMem = data;

			hr = device->CreateBuffer(&desc, &initData, &contantBuffer);
		}
		else
		{
			hr = device->CreateBuffer(&desc, nullptr, &contantBuffer);
		}

		if (FAILED(hr))
		{
			std::cout << "Failed to create vertex buffer\n";
		}
	}

	D3D11UniformBuffer::~D3D11UniformBuffer()
	{
		if (contantBuffer)
			contantBuffer->Release();
	}

	void D3D11UniformBuffer::Update(const void *data, unsigned int size, int offset)
	{
		assert(context);
		context->Map(contantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
		void *mapped = (char*)mappedResource.pData + offset;
		memcpy(mapped, data, size);
		context->Unmap(contantBuffer, 0);
	}

	void D3D11UniformBuffer::Map()
	{
		assert(context);
		context->Map(contantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
	}

	void D3D11UniformBuffer::UpdateMapped(const void *data, unsigned int size, int offset)
	{
		assert(mappedResource.pData);
		void *mapped = (char*)mappedResource.pData + offset;
		memcpy(mapped, data, size);
	}

	void D3D11UniformBuffer::Unmap()
	{
		context->Unmap(contantBuffer, 0);
	}
}
