#include "D3D11DrawIndirectBuffer.h"

#include "Program/Log.h"

namespace Engine
{
	D3D11DrawIndirectBuffer::D3D11DrawIndirectBuffer(ID3D11Device *device, ID3D11DeviceContext *context, const void *data, unsigned int size) : Buffer(BufferType::DrawIndirectBuffer)
	{
		buffer = nullptr;
		uav = nullptr;

		D3D11_BUFFER_DESC desc = {};
		desc.Usage = D3D11_USAGE_DEFAULT;
		desc.CPUAccessFlags = 0;
		desc.ByteWidth = size;
		desc.BindFlags = D3D11_BIND_UNORDERED_ACCESS;
		desc.MiscFlags = D3D11_RESOURCE_MISC_DRAWINDIRECT_ARGS;
		desc.StructureByteStride = 20;

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
			Log::Print(LogLevel::LEVEL_ERROR, "Failed to create draw indirect buffer");
			return;
		}

		D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
		uavDesc.Format = DXGI_FORMAT_R32_UINT;
		uavDesc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
		uavDesc.Buffer.FirstElement = 0;
		uavDesc.Buffer.Flags = 0;
		uavDesc.Buffer.NumElements = 5;

		hr = device->CreateUnorderedAccessView(buffer, &uavDesc, &uav);
		if (FAILED(hr))
		{
			buffer->Release();
			Log::Print(LogLevel::LEVEL_ERROR, "Failed to create draw indirect buffer unordered access view");
		}
	}

	D3D11DrawIndirectBuffer::~D3D11DrawIndirectBuffer()
	{
		if (buffer)
			buffer->Release();
		if (uav)
			uav->Release();
	}
}
