#pragma once

#include "Graphics/Framebuffer.h"

#include "psp2/gxm.h"

namespace Engine
{
	struct ColorSurface
	{
		SceGxmColorSurface surface;
		SceUID uid;
		void *addr;
		SceGxmSyncObject *syncObj;
	};

	struct DepthStencilSurface
	{
		SceGxmDepthStencilSurface surface;
		SceUID uid;
		void *addr;
	};

	class GXMFramebuffer : public Framebuffer
	{
	public:
		GXMFramebuffer(const FramebufferDesc &desc);
		~GXMFramebuffer();

		void Resize(const FramebufferDesc &desc) override;
		void Clear() const override {}
		void Bind() const override {}

		const ColorSurface &GetColorSurface(unsigned int index) const { return colorSurfaces[index]; }
		const DepthStencilSurface &GetDepthStencilSurface() const { return depthStencilSurface; }

		SceGxmRenderTarget *GetRTHandle() const { return renderTarget; }

	private:
		SceGxmRenderTarget *renderTarget;
		std::vector<ColorSurface> colorSurfaces;
		DepthStencilSurface depthStencilSurface;
	};
}
