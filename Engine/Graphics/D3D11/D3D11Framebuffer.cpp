#include "D3D11Framebuffer.h"

#include "D3D11Texture2D.h"

#include <iostream>

namespace Engine
{
	D3D11Framebuffer::D3D11Framebuffer(ID3D11Device *device, ID3D11DeviceContext *context, const FramebufferDesc &desc)
	{
		this->device = device;
		this->context = context;
		depthStencilView = nullptr;
		depthAttachment.texture = nullptr;

		width = desc.width;
		height = desc.height;
		useColor = desc.colorTextures.size() > 0;
		useDepth = desc.useDepth;
		colorOnly = useColor && !useDepth;
		writesDisabled = desc.writesDisabled;

		// If we don't write to this framebuffer then don't create it
		if(!writesDisabled)
			Create(desc);
	}

	D3D11Framebuffer::~D3D11Framebuffer()
	{
		Dispose();
	}

	void D3D11Framebuffer::Resize(const FramebufferDesc &desc)
	{
		if (desc.width == width && desc.height == height)
			return;

		if (writesDisabled)
			return;

		if (renderTargetViews.size() > 0 || depthStencilView != nullptr)
			Dispose();

		width = desc.width;
		height = desc.height;

		Create(desc);
	}

	void D3D11Framebuffer::Create(const FramebufferDesc &desc)
	{
		for (size_t i = 0; i < desc.colorTextures.size(); i++)
		{
			FramebufferAttachment attachment = {};
			attachment.params = desc.colorTextures[i];
			colorAttachments.push_back(attachment);
		}

		if (useColor)
		{
			for (size_t i = 0; i < colorAttachments.size(); i++)
			{
				D3D11Texture2D *tex = new D3D11Texture2D(device, context, width, height, colorAttachments[i].params, true);
				colorAttachments[i].texture = tex;

				D3D11_RENDER_TARGET_VIEW_DESC rtvDesc = {};
				rtvDesc.Format = tex->GetFormat();;
				rtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
				rtvDesc.Texture2D.MipSlice = 0;

				ID3D11RenderTargetView *rtv = nullptr;
				HRESULT hr = device->CreateRenderTargetView(tex->GetTextureHandle(), &rtvDesc, &rtv);

				if (FAILED(hr))
				{
					std::cout << "Failed to create render target view!\n";
					return;
				}

				renderTargetViews.push_back(rtv);
			}
		}

		if (useDepth)
		{
			depthAttachment.params = desc.depthTexture;
			D3D11Texture2D *tex = new D3D11Texture2D(device, context, width, height, depthAttachment.params, desc.sampleDepth);
			depthAttachment.texture = tex;

			D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
			dsvDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;			// Check formats
			dsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
			dsvDesc.Texture2D.MipSlice = 0;

			HRESULT hr = device->CreateDepthStencilView(tex->GetTextureHandle(), &dsvDesc, &depthStencilView);
			if (FAILED(hr))
			{
				std::cout << "Failed to create depth stencil view!\n";
				return;
			}
		}
	}

	void D3D11Framebuffer::Dispose()
	{
		for (size_t i = 0; i < colorAttachments.size(); i++)
		{
			colorAttachments[i].texture->RemoveReference();
		}
		colorAttachments.clear();

		if (depthAttachment.texture)
		{
			depthAttachment.texture->RemoveReference();
		}

		for (size_t i = 0; i < renderTargetViews.size(); i++)
		{
			if (renderTargetViews[i])
				renderTargetViews[i]->Release();
		}
		renderTargetViews.clear();

		if (depthStencilView)
			depthStencilView->Release();
	}
}
