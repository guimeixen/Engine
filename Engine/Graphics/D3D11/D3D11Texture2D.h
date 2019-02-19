#pragma once

#include "Graphics\Texture.h"

#include <d3d11_1.h>

namespace Engine
{
	class D3D11Texture2D : public Texture
	{
	public:
		D3D11Texture2D(ID3D11Device *device, ID3D11DeviceContext *context, const std::string &path, const TextureParams &params, bool storeTextureData = false);
		D3D11Texture2D(ID3D11Device *device, ID3D11DeviceContext *context, unsigned int width, unsigned int height, const TextureParams &params, bool sampleInShader);
		D3D11Texture2D(ID3D11Device *device, ID3D11DeviceContext *context, unsigned int width, unsigned int height, const TextureParams &params, const void *data);
		~D3D11Texture2D();

		ID3D11ShaderResourceView *GetShaderResourceView() const { return srv; }
		ID3D11UnorderedAccessView *GetUnorderedAcessView() const { return uav; }
		ID3D11SamplerState *GetSamplerState() const { return samplerState; }

		void Bind(unsigned int slot) const override;
		void BindAsImage(unsigned int slot, unsigned int mipLevel, bool layered, ImageAccess access, TextureInternalFormat format) const override;
		void Unbind(unsigned int slot) const override;

		unsigned int GetID() const override { return 0; }

		unsigned int GetWidth() const override { return width; }
		unsigned int GetHeight() const override { return height; }

		DXGI_FORMAT GetFormat() const { return format; }
		ID3D11Texture2D *GetTextureHandle() const { return tex; }

		void Clear() override;

	private:
		void LoadKTXDDS(ID3D11Device *device, ID3D11DeviceContext *context);
		void LoadPNGJPG(ID3D11Device *device, ID3D11DeviceContext *context);
		bool CreateSampler(ID3D11Device *device);
		bool CreateSRV(ID3D11Device *device, ID3D11DeviceContext *context);
		bool CreateUAV(ID3D11Device *device);

	private:
		unsigned int width;
		unsigned int height;
		ID3D11Texture2D *tex = nullptr;
		ID3D11ShaderResourceView *srv = nullptr;
		ID3D11UnorderedAccessView *uav = nullptr;
		ID3D11SamplerState *samplerState = nullptr;
		DXGI_FORMAT format;
		bool mipmapsGenerated;
	};
}
