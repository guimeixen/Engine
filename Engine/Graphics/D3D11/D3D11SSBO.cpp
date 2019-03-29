#include "D3D11SSBO.h"

#include "Program/Log.h"

namespace Engine
{
	D3D11SSBO::D3D11SSBO(ID3D11Device *device, ID3D11DeviceContext *context, const void *data, unsigned int size, unsigned int stride, BufferUsage usage) : Buffer(BufferType::ShaderStorageBuffer)
	{
		this->context = context;
		buffer = nullptr;
		srv = nullptr;
		uav = nullptr;

		D3D11_BUFFER_DESC desc = {};
		desc.ByteWidth = size;

		if (usage == BufferUsage::DYNAMIC)
		{
			desc.Usage = D3D11_USAGE_DYNAMIC;
			desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
			desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		}
		else
		{
			desc.Usage = D3D11_USAGE_DEFAULT;
			desc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS;
			desc.CPUAccessFlags = 0;
		}

		desc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
		desc.StructureByteStride = stride;

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
			Log::Print(LogLevel::LEVEL_ERROR, "Failed to create ssbo");
			return;
		}

		D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.Format = DXGI_FORMAT_UNKNOWN;
		srvDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
		srvDesc.Buffer.ElementWidth = size / stride;

		hr = device->CreateShaderResourceView(buffer, &srvDesc, &srv);

		if (FAILED(hr))
		{
			if (buffer)
			{
				buffer->Release();
				buffer = nullptr;
			}
			Log::Print(LogLevel::LEVEL_ERROR, "Failed to create ssbo srv");
		}

		if (usage == BufferUsage::STATIC)
		{
			D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
			uavDesc.Format = DXGI_FORMAT_UNKNOWN;
			uavDesc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
			uavDesc.Buffer.FirstElement = 0;
			uavDesc.Buffer.Flags = 0;
			uavDesc.Buffer.NumElements = size / stride;

			hr = device->CreateUnorderedAccessView(buffer, &uavDesc, &uav);
			if (FAILED(hr))
			{
				if (buffer)
				{
					buffer->Release();
					buffer = nullptr;
				}
				if (srv)
				{
					srv->Release();
					srv = nullptr;
				}
				Log::Print(LogLevel::LEVEL_ERROR, "Failed to create ssbo unordered access view");
			}
		}
	}

	D3D11SSBO::~D3D11SSBO()
	{
		if (buffer)
			buffer->Release();
		if (srv)
			srv->Release();
		if (uav)
			uav->Release();
	}

	void D3D11SSBO::BindTo(unsigned int bindingIndex)
	{
	}

	void D3D11SSBO::Update(const void *data, unsigned int size, int offset)
	{
		assert(mappedResource.pData);
		void *mapped = (char*)mappedResource.pData + offset;
		memcpy(mapped, data, size);
	}

	void D3D11SSBO::Map()
	{
		assert(context);
		
		context->Map(buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
	}

	void D3D11SSBO::Unmap()
	{
		context->Unmap(buffer, 0);
	}
}
