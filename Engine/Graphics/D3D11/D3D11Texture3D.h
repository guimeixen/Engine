#pragma once

#include "Graphics\Texture.h"

#include <d3d11_1.h>

namespace Engine
{
	class D3D11Texture3D : public Texture
	{
	public:
		D3D11Texture3D(ID3D11Device *device, unsigned int width, unsigned int height, unsigned int depth, const TextureParams &params, const void *data);
		~D3D11Texture3D();

		DXGI_FORMAT GetFormat() const { return format; }
		ID3D11Texture3D *GetTextureHandle() const { return tex; }
		ID3D11ShaderResourceView *GetShaderResourceView() const { return srv; }
		ID3D11UnorderedAccessView *GetUAV() const { return uav; }
		ID3D11SamplerState *GetSamplerState() const { return samplerState; }

		void Bind(unsigned int slot) const override {}
		void BindAsImage(unsigned int slot, unsigned int mipLevel, bool layered, ImageAccess access, TextureInternalFormat format) const override {}
		void Unbind(unsigned int slot) const override {}

		unsigned int GetID() const override { return 0; }

		unsigned int GetWidth() const override { return width; }
		unsigned int GetHeight() const override { return height; }

		void Clear() override {}

	private:
		bool CreateSampler(ID3D11Device *device);
		bool CreateSRV(ID3D11Device *device);
		bool CreateUAV(ID3D11Device *device);

	private:
		unsigned int width;
		unsigned int height;
		unsigned int depth;
		ID3D11Texture3D *tex = nullptr;
		ID3D11ShaderResourceView *srv = nullptr;
		ID3D11UnorderedAccessView *uav = nullptr;
		ID3D11SamplerState *samplerState = nullptr;
		DXGI_FORMAT format;
	};
}
