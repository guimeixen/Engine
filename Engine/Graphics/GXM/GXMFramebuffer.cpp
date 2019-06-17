#include "GXMFramebuffer.h"

#include "GXMUtils.h"

#include <cstring>	// for memset

#define DISPLAY_COLOR_FORMAT SCE_GXM_COLOR_FORMAT_A8B8G8R8
#define DISPLAY_STRIDE_IN_PIXELS	 1024

namespace Engine
{
	GXMFramebuffer::GXMFramebuffer(const FramebufferDesc &desc)
	{
		this->width = desc.width;
		this->height = desc.height;
		useDepth = desc.useDepth;

		SceGxmRenderTargetParams rtParams = {};
		rtParams.flags = 0;
		rtParams.width = (uint16_t)width;
		rtParams.height = (uint16_t)height;
		rtParams.scenesPerFrame = 1;
		rtParams.multisampleMode = SCE_GXM_MULTISAMPLE_NONE;
		rtParams.multisampleLocations = 0;
		rtParams.driverMemBlock = -1;

		sceGxmCreateRenderTarget(&rtParams, &renderTarget);

		for (size_t i = 0; i < desc.colorTextures.size(); i++)
		{
			ColorSurface cs = {};
			cs.addr = gxmutils::graphicsAlloc(SCE_KERNEL_MEMBLOCK_TYPE_USER_CDRAM_RW, (SceGxmMemoryAttribFlags)(SCE_GXM_MEMORY_ATTRIB_READ | SCE_GXM_MEMORY_ATTRIB_WRITE), ALIGN(4 * DISPLAY_STRIDE_IN_PIXELS * height, 1 * 1024 * 1024), &cs.uid);
			memset(cs.addr, 0, DISPLAY_STRIDE_IN_PIXELS * height);

			sceGxmColorSurfaceInit(&cs.surface, DISPLAY_COLOR_FORMAT, SCE_GXM_COLOR_SURFACE_LINEAR, SCE_GXM_COLOR_SURFACE_SCALE_NONE, SCE_GXM_OUTPUT_REGISTER_SIZE_32BIT, width, height, DISPLAY_STRIDE_IN_PIXELS, cs.addr);
			sceGxmSyncObjectCreate(&cs.syncObj);

			colorSurfaces.push_back(cs);
		}

		if (desc.useDepth)
		{
			unsigned int depthStencilWidth = ALIGN(width, SCE_GXM_TILE_SIZEX);
			unsigned int depthStencilHeight = ALIGN(height, SCE_GXM_TILE_SIZEY);
			unsigned int depthStencilSamples = depthStencilWidth * depthStencilHeight;

			depthStencilSurface.addr = gxmutils::graphicsAlloc(SCE_KERNEL_MEMBLOCK_TYPE_USER_RW_UNCACHE, (SceGxmMemoryAttribFlags)(SCE_GXM_MEMORY_ATTRIB_READ | SCE_GXM_MEMORY_ATTRIB_WRITE), 4 * depthStencilSamples, &depthStencilSurface.uid);

			sceGxmDepthStencilSurfaceInit(&depthStencilSurface.surface, SCE_GXM_DEPTH_STENCIL_FORMAT_S8D24, SCE_GXM_DEPTH_STENCIL_SURFACE_TILED, depthStencilWidth, depthStencilSurface.addr, nullptr);
		}
	}

	GXMFramebuffer::~GXMFramebuffer()
	{
		gxmutils::graphicsFree(depthStencilSurface.uid);
		for (unsigned int i = 0; i < colorSurfaces.size(); ++i)
		{
			ColorSurface &cs = colorSurfaces[i];

			// clear the buffer then deallocate
			memset(cs.addr, 0, height * DISPLAY_STRIDE_IN_PIXELS * 4);
			gxmutils::graphicsFree(cs.uid);

			// destroy the sync object
			sceGxmSyncObjectDestroy(cs.syncObj);
		}

		sceGxmDestroyRenderTarget(renderTarget);
	}

	void GXMFramebuffer::Resize(const FramebufferDesc &desc)
	{
	}
}
