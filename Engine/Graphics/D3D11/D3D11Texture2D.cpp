#include "D3D11Texture2D.h"

#include "D3D11Utils.h"
#include "Program\Log.h"

#include "include\gli\gli.hpp"
#include "include\stb_image.h"
#include "include\half.hpp"

#include <iostream>

namespace Engine
{
	D3D11Texture2D::D3D11Texture2D(ID3D11Device *device, ID3D11DeviceContext *context, const std::string &path, const TextureParams &params, bool storeTextureData)
	{
		tex = nullptr;
		srv = nullptr;
		samplerState = nullptr;
		data = nullptr;
		this->path = path;
		this->params = params;
		type = TextureType::TEXTURE2D;
		this->storeTextureData = storeTextureData;
		mipmapsGenerated = false;

		if (std::strstr(path.c_str(), ".ktx") > 0 || std::strstr(path.c_str(), ".dds") > 0)
			LoadKTXDDS(device, context);
		//else if (std::strstr(path.c_str(), ".raw") > 0)
		//	LoadRaw();
		else if (std::strstr(path.c_str(), ".png") > 0 || std::strstr(path.c_str(), ".jpg") > 0)
			LoadPNGJPG(device, context);
	}

	D3D11Texture2D::D3D11Texture2D(ID3D11Device *device, ID3D11DeviceContext *context, unsigned int width, unsigned int height, const TextureParams &params, bool sampleInShader)
	{
		tex = nullptr;
		srv = nullptr;
		samplerState = nullptr;
		data = nullptr;		
		type = TextureType::TEXTURE2D;
		storeTextureData = false;
		this->params = params;
		this->width = width;
		this->height = height;
		mipmapsGenerated = false;

		format = d3d11utils::GetFormat(params.internalFormat);
		
		D3D11_TEXTURE2D_DESC rtDesc = {};
		rtDesc.Width = width;
		rtDesc.Height = height;
		rtDesc.MipLevels = 1;
		rtDesc.ArraySize = 1;
		rtDesc.Format = format;
		rtDesc.SampleDesc.Count = 1;
		rtDesc.SampleDesc.Quality = 0;
		rtDesc.Usage = D3D11_USAGE_DEFAULT;
		rtDesc.CPUAccessFlags = 0;
		rtDesc.MiscFlags = 0;

		if (Texture::IsDepthTexture(params.internalFormat))
		{
			format = DXGI_FORMAT_R24G8_TYPELESS;				// Check formats
			rtDesc.Format = format;
			rtDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
		}
		else
		{
			rtDesc.BindFlags = D3D11_BIND_RENDER_TARGET;
		}
		if (sampleInShader)
			rtDesc.BindFlags |= D3D11_BIND_SHADER_RESOURCE;

		HRESULT hr = device->CreateTexture2D(&rtDesc, nullptr, &tex);
		if (FAILED(hr))
		{
			Log::Print(LogLevel::LEVEL_ERROR, "Failed to create render target texture");
			return;
		}

		if (sampleInShader)
		{
			if (!CreateSampler(device))
				return;
			if (!CreateSRV(device, context))
				return;
		}
	}

	D3D11Texture2D::D3D11Texture2D(ID3D11Device *device, ID3D11DeviceContext *context, unsigned int width, unsigned int height, const TextureParams &params, const void *data)
	{
		AddReference();
		tex = nullptr;
		srv = nullptr;
		samplerState = nullptr;
		this->data = nullptr;
		type = TextureType::TEXTURE2D;
		storeTextureData = false;
		this->params = params;
		this->width = width;
		this->height = height;
		mipLevels = 1;
		mipmapsGenerated = false;

		format = d3d11utils::GetFormat(params.internalFormat);

		D3D11_TEXTURE2D_DESC texDesc = {};
		texDesc.Width = width;
		texDesc.Height = height;
		texDesc.MipLevels = mipLevels;
		texDesc.ArraySize = 1;
		texDesc.Format = format;
		texDesc.SampleDesc.Count = 1;
		texDesc.SampleDesc.Quality = 0;
		texDesc.Usage = D3D11_USAGE_DEFAULT;
		texDesc.CPUAccessFlags = 0;
		texDesc.MiscFlags = 0;

		if (params.usedAsStorageInCompute || params.usedAsStorageInGraphics)
			texDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS;
		else
			texDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

		unsigned int numChannels = Texture::GetNumChannels(params.internalFormat);

		if (data)
		{
			D3D11_SUBRESOURCE_DATA sd = {};
			if (params.type == TextureDataType::FLOAT)
			{
				sd.SysMemPitch = width * numChannels * sizeof(float);
			}
			else if (params.type == TextureDataType::UNSIGNED_BYTE)
			{
				sd.SysMemPitch = width * numChannels * sizeof(unsigned char);
			}

			// Because D3D11 doesn't support formats with 24 bits we convert to 32 bits
			// This assumes we're using RGB16F. Change texture data type to HALF_FLOAT?
			if (numChannels == 3 && params.type == TextureDataType::FLOAT)
			{
				numChannels = 4;
				sd.SysMemPitch = width * numChannels * sizeof(half_float::half);

				half_float::half *newData = new half_float::half[width * height * 4];
				const float *oldData = static_cast<const float*>(data);
				unsigned int counter = 0;
				unsigned int counterOld = 0;

				// Better way to copy?
				for (unsigned int i = 0; i < width * height; i++)
				{
					newData[counter++] = half_float::half_cast<half_float::half>(oldData[counterOld++]);
					newData[counter++] = half_float::half_cast<half_float::half>(oldData[counterOld++]);
					newData[counter++] = half_float::half_cast<half_float::half>(oldData[counterOld++]);
					newData[counter++] = half_float::half_cast<half_float::half>(0.0f);
				}

				sd.pSysMem = newData;

				HRESULT hr = device->CreateTexture2D(&texDesc, &sd, &tex);
				if (FAILED(hr))
				{
					Log::Print(LogLevel::LEVEL_ERROR, "Failed to create texture");
					return;
				}

				delete[] newData;
			}
			else
			{
				sd.pSysMem = data;

				HRESULT hr = device->CreateTexture2D(&texDesc, &sd, &tex);
				if (FAILED(hr))
				{
					Log::Print(LogLevel::LEVEL_ERROR, "Failed to create texture");
					return;
				}
			}
		}
		else
		{
			HRESULT hr = device->CreateTexture2D(&texDesc, nullptr, &tex);
			if (FAILED(hr))
			{
				Log::Print(LogLevel::LEVEL_ERROR, "Failed to create empty texture");
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
		
		/*if()
		{*/
			if (!CreateSRV(device, context))
				return;
		//}	
	}

	D3D11Texture2D::~D3D11Texture2D()
	{
		if (tex)
			tex->Release();

		if (srv)
			srv->Release();

		if (samplerState)
			samplerState->Release();

		if (data)
		{
			delete data;
			data = nullptr;
		}
	}

	void D3D11Texture2D::Bind(unsigned int slot) const
	{
	}

	void D3D11Texture2D::BindAsImage(unsigned int slot, unsigned int mipLevel, bool layered, ImageAccess access, TextureInternalFormat format) const
	{
	}

	void D3D11Texture2D::Unbind(unsigned int slot) const
	{
	}

	void D3D11Texture2D::Clear()
	{
	}

	void D3D11Texture2D::LoadKTXDDS(ID3D11Device *device, ID3D11DeviceContext *context)
	{
		/*if (std::strstr(path.c_str(), ".ktx") > 0)
			format = DXGI_FORMAT_BC2_UNORM;
		else
		{
			std::cout << "Error -> Unsupported image format!\n";
			return;
		}*/

		gli::texture2d tex2D(gli::load(path));
		if (tex2D.empty())
		{
			Log::Print(LogLevel::LEVEL_ERROR, "Failed to load texture");
			return;
		}

		width = tex2D[0].extent().x;
		height = tex2D[0].extent().y;
		mipLevels = tex2D.levels();
		unsigned int size = tex2D.size();
		mipmapsGenerated = true;

		gli::format gliformat = tex2D.format();

		if (gliformat == gli::FORMAT_RGBA_DXT1_UNORM_BLOCK8)
		{
			format = DXGI_FORMAT_BC1_UNORM;
		}
		else if (gliformat == gli::FORMAT_RGBA_DXT5_UNORM_BLOCK16)
		{
			format = DXGI_FORMAT_BC3_UNORM;
		}

		D3D11_TEXTURE2D_DESC texDesc = {};
		texDesc.Width = width;
		texDesc.Height = height;
		texDesc.MipLevels = mipLevels;
		texDesc.ArraySize = 1;
		texDesc.Format = format;
		texDesc.SampleDesc.Count = 1;
		texDesc.SampleDesc.Quality = 0;
		texDesc.Usage = D3D11_USAGE_DEFAULT;
		texDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
		texDesc.CPUAccessFlags = 0;
		texDesc.MiscFlags = 0;

		std::vector<D3D11_SUBRESOURCE_DATA> initData(mipLevels);
		for (unsigned int i = 0; i < mipLevels; i++)
		{
			D3D11_SUBRESOURCE_DATA sd = {};
			sd.pSysMem = tex2D[i].data();
			sd.SysMemPitch = tex2D[i].extent().x * 1;		// The distance (in bytes) from the beginning of one line of a texture to the next line
			//initData.SysMemSlicePitch = 
			initData[i] = sd;
		}

		HRESULT hr = device->CreateTexture2D(&texDesc, initData.data(), &tex);
		if (FAILED(hr))
		{
			Log::Print(LogLevel::LEVEL_ERROR, "Failed to create create texture");
			return;
		}

		if (!CreateSampler(device))
			return;
		if (!CreateSRV(device, context))
			return;
	}

	void Engine::D3D11Texture2D::LoadPNGJPG(ID3D11Device *device, ID3D11DeviceContext *context)
	{
		unsigned char *image = nullptr;

		int texWidth = 0;
		int textHeight = 0;
		int channelsInFile = 0;
		int channelsInData = 0;

		if (params.format == TextureFormat::RGB)
		{
			image = stbi_load(path.c_str(), &texWidth, &textHeight, &channelsInFile, STBI_rgb_alpha);		// Force RGBA, because 24bits is not supported
			channelsInData = 4;
		}
		else if (params.format == TextureFormat::RGBA)
		{
			image = stbi_load(path.c_str(), &texWidth, &textHeight, &channelsInFile, STBI_rgb_alpha);
			channelsInData = 4;
		}
		else if (params.format == TextureFormat::RED)
		{
			image = stbi_load(path.c_str(), &texWidth, &textHeight, &channelsInFile, STBI_grey);
			channelsInData = 1;
		}

		if (!image)
		{
			Log::Print(LogLevel::LEVEL_ERROR, "Failed to load texture: %s", path.c_str());
			return;
		}

		width = static_cast<unsigned int>(texWidth);
		height = static_cast<unsigned int>(textHeight);
		format = d3d11utils::GetFormat(params.internalFormat);
		mipLevels = 1;

		if (storeTextureData)
		{
			if (channelsInData == 1)
			{
				data = new unsigned char[width * height];
				memcpy(data, image, width * height * sizeof(unsigned char));
			}
			else if (channelsInData == 4)
			{
				data = new unsigned char[width * height * 4];
				memcpy(data, image, width * height * 4 * sizeof(unsigned char));
			}
		}
		

		D3D11_TEXTURE2D_DESC texDesc = {};
		texDesc.Width = width;
		texDesc.Height = height;
		texDesc.MipLevels = mipLevels;
		texDesc.ArraySize = 1;
		texDesc.Format = format;
		texDesc.SampleDesc.Count = 1;
		texDesc.SampleDesc.Quality = 0;
		texDesc.Usage = D3D11_USAGE_DEFAULT;
		texDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
		texDesc.CPUAccessFlags = 0;
		texDesc.MiscFlags = 0;

		if (params.useMipmapping)
		{
			texDesc.BindFlags |= D3D11_BIND_RENDER_TARGET;
			texDesc.MiscFlags = D3D11_RESOURCE_MISC_GENERATE_MIPS;
			mipLevels = (unsigned int)std::floor(std::log2(std::max(width, height))) + 1;
			texDesc.MipLevels = mipLevels;
		}

		std::vector<D3D11_SUBRESOURCE_DATA> initData(mipLevels);
		int mipWidth = width;
		for (unsigned int i = 0; i < mipLevels; i++)
		{
			D3D11_SUBRESOURCE_DATA sd = {};
			sd.pSysMem = image;
			sd.SysMemPitch = mipWidth * channelsInData;
			initData[i] = sd;
			mipWidth = mipWidth >> 1;
			mipWidth = std::max(mipWidth, 1);
		}

		HRESULT hr = device->CreateTexture2D(&texDesc, initData.data(), &tex);
		if (FAILED(hr))
		{
			Log::Print(LogLevel::LEVEL_ERROR, "Failed to create texture");
			return;
		}

		if (!CreateSampler(device))
			return;
		if (!CreateSRV(device, context))
			return;

		stbi_image_free(image);
	}

	bool D3D11Texture2D::CreateSampler(ID3D11Device *device)
	{
		D3D11_SAMPLER_DESC samplerDesc = {};

		if (params.useMipmapping)
		{
			samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
		}
		else
		{
			if (params.enableCompare)
			{
				if (params.filter == TextureFilter::LINEAR)
					samplerDesc.Filter = D3D11_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT;
				else if (params.filter == TextureFilter::NEAREST)
					samplerDesc.Filter = D3D11_FILTER_COMPARISON_MIN_MAG_MIP_POINT;

				samplerDesc.BorderColor[0] = 1.0f;
				samplerDesc.BorderColor[1] = 1.0f;
				samplerDesc.BorderColor[2] = 1.0f;
				samplerDesc.BorderColor[3] = 1.0f;
			}
			else
			{
				if (params.filter == TextureFilter::LINEAR)
					samplerDesc.Filter = D3D11_FILTER_MIN_MAG_LINEAR_MIP_POINT;
				else if (params.filter == TextureFilter::NEAREST)
					samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
			}		
		}

		D3D11_TEXTURE_ADDRESS_MODE mode = d3d11utils::GetAddressMode(params.wrap);
		
		samplerDesc.AddressU = mode;
		samplerDesc.AddressV = mode;
		samplerDesc.AddressW = mode;
		samplerDesc.MipLODBias = 0.0f;
		samplerDesc.MaxAnisotropy = 1;

		if(params.enableCompare)
			samplerDesc.ComparisonFunc = D3D11_COMPARISON_LESS;
		else
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
			Log::Print(LogLevel::LEVEL_ERROR, "Failed to create sampler state");
			return false;
		}

		return true;
	}

	bool D3D11Texture2D::CreateSRV(ID3D11Device *device, ID3D11DeviceContext *context)
	{
		D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};

		if (Texture::IsDepthTexture(params.internalFormat))
			srvDesc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
		else
			srvDesc.Format = format;

		srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MipLevels = mipLevels;
		srvDesc.Texture2D.MostDetailedMip = 0;

		HRESULT hr = device->CreateShaderResourceView(tex, &srvDesc, &srv);
		if (FAILED(hr))
		{
			tex->Release();
			samplerState->Release();
			Log::Print(LogLevel::LEVEL_ERROR, "Failed to create shader resource view");
			return false;
		}

		if (params.useMipmapping && mipmapsGenerated == false)
		{
			context->GenerateMips(srv);
			mipmapsGenerated = true;
		}

		return true;
	}

	bool D3D11Texture2D::CreateUAV(ID3D11Device *device)
	{
		D3D11_UNORDERED_ACCESS_VIEW_DESC desc = {};
		desc.Format = format;
		desc.Texture2D.MipSlice = 0;
		desc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2D;

		HRESULT hr = device->CreateUnorderedAccessView(tex, &desc, &uav);
		if (FAILED(hr))
		{
			tex->Release();
			samplerState->Release();
			Log::Print(LogLevel::LEVEL_ERROR, "Failed to create texture2d unordered access view");
			return false;
		}

		return true;
	}
}
