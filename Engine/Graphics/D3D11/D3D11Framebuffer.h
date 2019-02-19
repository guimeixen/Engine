#pragma once

#include "Graphics\Framebuffer.h"

#include <d3d11_1.h>

namespace Engine
{
	class D3D11Framebuffer : public Framebuffer
	{
	public:
		D3D11Framebuffer(ID3D11Device *device, ID3D11DeviceContext *context, const FramebufferDesc &desc);
		~D3D11Framebuffer();

		const std::vector<ID3D11RenderTargetView*> &GetRenderTargetViews() const { return renderTargetViews; }
		ID3D11DepthStencilView *GetDepthStencilView() const { return depthStencilView; }

		void Resize(const FramebufferDesc &desc) override;
		void Clear() const override {}
		void Bind() const override {}

	private:
		void Create(const FramebufferDesc &desc);
		void Dispose();

	private:
		ID3D11Device *device;
		ID3D11DeviceContext *context;
		std::vector<ID3D11RenderTargetView*> renderTargetViews;
		ID3D11DepthStencilView *depthStencilView;
	};
}
