#include "D3D11Texture3D.h"

#include "D3D11Utils.h"
#include "Program/Log.h"

#include "include/half.hpp"

namespace Engine
{
	D3D11Texture3D::D3D11Texture3D(ID3D11Device *device, unsigned int width, unsigned int height, unsigned int depth, const TextureParams &params, const void *data)
	{
		AddReference();
		this->data = nullptr;
		this->params = params;
		this->width = width;
		this->height = height;
		this->depth = depth;
		this->storeTextureData = false;
		isAttachment = false;
		type = TextureType::TEXTURE3D;
		mipLevels = 1;

		format = d3d11utils::GetFormat(params.internalFormat);

		D3D11_TEXTURE3D_DESC texDesc = {};
		texDesc.Width = width;
		texDesc.Height = height;
		texDesc.Depth = depth;
		texDesc.MipLevels = mipLevels;
		texDesc.Format = format;
		texDesc.Usage = D3D11_USAGE_DEFAULT;

		if (params.usedAsStorageInCompute || params.usedAsStorageInGraphics)
		{
			texDesc.BindFlags = D3D11_BIND_UNORDERED_ACCESS;
		}
		else
		{
			texDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
		}

		
		texDesc.CPUAccessFlags = 0;
		texDesc.MiscFlags = 0;

		unsigned int numChannels = Texture::GetNumChannels(params.internalFormat);

		if (data)
		{
			D3D11_SUBRESOURCE_DATA sd = {};
			if (params.type == TextureDataType::FLOAT)
			{
				sd.SysMemPitch = width * numChannels * sizeof(float);
				sd.SysMemSlicePitch = width * height * numChannels * sizeof(float);
			}
			else if (params.type == TextureDataType::UNSIGNED_BYTE)
			{
				sd.SysMemPitch = width * numChannels * sizeof(unsigned char);
				sd.SysMemSlicePitch = width * height * numChannels * sizeof(unsigned char);
			}

			// Because Vulkan doesn't support formats with 24 bits we convert to 32 bits
			if (numChannels == 3 && params.type == TextureDataType::FLOAT)
			{
				// copy to 32 bits
			}
			else if (numChannels == 4 && params.type == TextureDataType::FLOAT)
			{
				sd.SysMemPitch = width * numChannels * sizeof(half_float::half);
				sd.SysMemSlicePitch = width * height * numChannels * sizeof(half_float::half);

				half_float::half *newData = new half_float::half[width * height * depth * 4];
				const float *oldData = static_cast<const float*>(data);
				unsigned int counter = 0;
				unsigned int counterOld = 0;

				// Better way to copy?
				for (unsigned int i = 0; i < width * height * depth; i++)
				{
					newData[counter++] = half_float::half_cast<half_float::half>(oldData[counterOld++]);
					newData[counter++] = half_float::half_cast<half_float::half>(oldData[counterOld++]);
					newData[counter++] = half_float::half_cast<half_float::half>(oldData[counterOld++]);
					newData[counter++] = half_float::half_cast<half_float::half>(oldData[counterOld++]);
				}

				sd.pSysMem = newData;

				HRESULT hr = device->CreateTexture3D(&texDesc, &sd, &tex);
				if (FAILED(hr))
				{
					std::cout << "Failed to create 3d texture\n";
					return;
				}

				delete[] newData;
			}
			else
			{
				sd.pSysMem = data;

				HRESULT hr = device->CreateTexture3D(&texDesc, &sd, &tex);
				if (FAILED(hr))
				{
					std::cout << "Failed to create 3d texture\n";
					return;
				}
			}
		}
		else
		{
			HRESULT hr = device->CreateTexture3D(&texDesc, nullptr, &tex);
			if (FAILED(hr))
			{
				std::cout << "Failed to create empty 3d texture\n";
				return;
			}
		}

		if (!CreateSampler(device))
			return;

		if (params.usedAsStorageInCompute || params.usedAsStorageInGraphics)
		{
			if (!CreateUAV(device))
				return;
		}
		else
		{
			if (!CreateSRV(device))
				return;
		}
		
	}

	D3D11Texture3D::~D3D11Texture3D()
	{
		if (tex)
			tex->Release();
		if (samplerState)
			samplerState->Release();
		if (srv)
			srv->Release();
		if (uav)
			uav->Release();
	}

	bool D3D11Texture3D::CreateSampler(ID3D11Device *device)
	{
		D3D11_SAMPLER_DESC samplerDesc = {};

		if (params.useMipmapping)
		{
			samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
		}
		else
		{
			if (params.filter == TextureFilter::LINEAR)
				samplerDesc.Filter = D3D11_FILTER_MIN_MAG_LINEAR_MIP_POINT;
			else if (params.filter == TextureFilter::NEAREST)
				samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
		}

		D3D11_TEXTURE_ADDRESS_MODE mode = d3d11utils::GetAddressMode(params.wrap);

		samplerDesc.AddressU = mode;
		samplerDesc.AddressV = mode;
		samplerDesc.AddressW = mode;
		samplerDesc.MipLODBias = 0.0f;
		samplerDesc.MaxAnisotropy = 1;
		samplerDesc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
		samplerDesc.BorderColor[0] = 0;
		samplerDesc.BorderColor[1] = 0;
		samplerDesc.BorderColor[2] = 0;
		samplerDesc.BorderColor[3] = 0;
		samplerDesc.MinLOD = 0;
		samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;

		HRESULT hr = device->CreateSamplerState(&samplerDesc, &samplerState);
		if (FAILED(hr))
		{
			std::cout << "ERROR -> Failed to create sampler state\n";
			return false;
		}

		return true;
	}

	bool D3D11Texture3D::CreateSRV(ID3D11Device *device)
	{
		D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};

		if (Texture::IsDepthTexture(params.internalFormat))
			srvDesc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
		else
			srvDesc.Format = format;

		srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE3D;
		srvDesc.Texture3D.MipLevels = mipLevels;
		srvDesc.Texture3D.MostDetailedMip = 0;

		HRESULT hr = device->CreateShaderResourceView(tex, &srvDesc, &srv);
		if (FAILED(hr))
		{
			tex->Release();
			samplerState->Release();
			std::cout << "ERROR -> Failed to create shader resource view\n";
			return false;
		}

		return true;
	}

	bool D3D11Texture3D::CreateUAV(ID3D11Device *device)
	{
		D3D11_UNORDERED_ACCESS_VIEW_DESC desc = {};
		desc.Format = format;
		desc.Texture3D.FirstWSlice = 0;
		desc.Texture3D.MipSlice = 0;
		desc.Texture3D.WSize = depth;
		desc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE3D;

		HRESULT hr = device->CreateUnorderedAccessView(tex, &desc, &uav);
		if (FAILED(hr))
		{
			tex->Release();
			samplerState->Release();
			Log::Print(LogLevel::LEVEL_ERROR, "Failed to create texture3D unordered access view");
			return false;
		}

		return true;
	}
}
