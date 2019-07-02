#include "GXMFramebuffer.h"

#include "GXMTexture2D.h"
#include "GXMUtils.h"
#include "Program/Log.h"

#include <cstring>	// for memset

#define MAX(a, b)					(((a) > (b)) ? (a) : (b))

namespace Engine
{
	GXMFramebuffer::GXMFramebuffer(const FramebufferDesc &desc)
	{
		this->width = desc.width;
		this->height = desc.height;
		useColor = desc.colorTextures.size() > 0;
		useDepth = desc.useDepth;	
		colorOnly = useColor && !useDepth;
		writesDisabled = desc.writesDisabled;

		colorSurfaces.resize(desc.colorTextures.size());
		colorAttachments.resize(desc.colorTextures.size());

		depthAttachment = {};
		depthAttachment.params = desc.depthTexture;
		depthAttachment.texture = nullptr;

		for (size_t i = 0; i < desc.colorTextures.size(); i++)
		{
			// Check number of components instead of * 4
			const unsigned int colorDataSize = ALIGN(width * height * 4, MAX(SCE_GXM_TEXTURE_ALIGNMENT, SCE_GXM_COLOR_SURFACE_ALIGNMENT));

			ColorSurface cs = {};
			cs.data = gxmutils::graphicsAlloc(SCE_KERNEL_MEMBLOCK_TYPE_USER_CDRAM_RW, SCE_GXM_MEMORY_ATTRIB_RW, colorDataSize, &cs.uid);
			Log::Print(LogLevel::LEVEL_INFO, "Allocated %.2f mb for color surface\n", colorDataSize / 1024.0f / 1024.0f);

			sceGxmColorSurfaceInit(&cs.surface, SCE_GXM_COLOR_FORMAT_A8R8G8B8, SCE_GXM_COLOR_SURFACE_LINEAR, SCE_GXM_COLOR_SURFACE_SCALE_NONE, SCE_GXM_OUTPUT_REGISTER_SIZE_32BIT, width, height, width, cs.data);
			Log::Print(LogLevel::LEVEL_INFO, "Init color surface\n");

			//sceGxmSyncObjectCreate(&cs.syncObj);

			FramebufferAttachment attachment = {};
			attachment.params = desc.colorTextures[i];
			attachment.texture = new GXMTexture2D(width, height, attachment.params, cs.data);
			
			colorAttachments[i] = attachment;
			colorSurfaces[i] = cs;
		}

		if (desc.sampleDepth)
		{
			const unsigned int depthStencilAlignedWidth = ALIGN(width, SCE_GXM_TILE_SIZEX);
			const unsigned int depthStencilAlignedHeight = ALIGN(height, SCE_GXM_TILE_SIZEY);
			//const unsigned int alignment = MAX(SCE_GXM_TEXTURE_ALIGNMENT, SCE_GXM_DEPTHSTENCIL_SURFACE_ALIGNMENT);		// Same

			depthStencilSurface.data = gxmutils::graphicsAlloc(SCE_KERNEL_MEMBLOCK_TYPE_USER_CDRAM_RW, SCE_GXM_MEMORY_ATTRIB_RW, depthStencilAlignedWidth * depthStencilAlignedHeight * 4, &depthStencilSurface.uid);
			Log::Print(LogLevel::LEVEL_INFO, "Allocated %.2f mb for depth surface\n", depthStencilAlignedWidth * depthStencilAlignedHeight * 4 / 1024.0f / 1024.0f);

			sceGxmDepthStencilSurfaceInit(&depthStencilSurface.surface, SCE_GXM_DEPTH_STENCIL_FORMAT_DF32, SCE_GXM_DEPTH_STENCIL_SURFACE_TILED, depthStencilAlignedWidth, depthStencilSurface.data, nullptr);
			Log::Print(LogLevel::LEVEL_INFO, "Init depth surface\n");

			// Force the surface to always write data to memory because we want to use it as a texture and sample it in the shader
			sceGxmDepthStencilSurfaceSetForceStoreMode(&depthStencilSurface.surface, SCE_GXM_DEPTH_STENCIL_FORCE_STORE_ENABLED);

			depthAttachment.texture = new GXMTexture2D(width, height, depthAttachment.params, depthStencilSurface.data);
		}
		else if (desc.useDepth)
		{
			const unsigned int depthStencilAlignedWidth = ALIGN(width, SCE_GXM_TILE_SIZEX);
			const unsigned int depthStencilAlignedHeight = ALIGN(height, SCE_GXM_TILE_SIZEY);
			const unsigned int depthStencilSamples = depthStencilAlignedWidth * depthStencilAlignedHeight;
			const unsigned int depthStrideInSamples = depthStencilAlignedWidth;

			depthStencilSurface.data = gxmutils::graphicsAlloc(SCE_KERNEL_MEMBLOCK_TYPE_USER_RW_UNCACHE, SCE_GXM_MEMORY_ATTRIB_RW, 4 * depthStencilSamples, &depthStencilSurface.uid);
			Log::Print(LogLevel::LEVEL_INFO, "Allocated %.2f mb for depth surface\n", 4 * depthStencilSamples / 1024.0f / 1024.0f);

			sceGxmDepthStencilSurfaceInit(&depthStencilSurface.surface, SCE_GXM_DEPTH_STENCIL_FORMAT_S8D24, SCE_GXM_DEPTH_STENCIL_SURFACE_TILED, depthStrideInSamples, depthStencilSurface.data, nullptr);
			Log::Print(LogLevel::LEVEL_INFO, "Init depth surface\n");
		}

		SceGxmRenderTargetParams rtParams = {};
		rtParams.flags = 0;
		rtParams.width = (uint16_t)width;
		rtParams.height = (uint16_t)height;
		rtParams.scenesPerFrame = 1;
		rtParams.multisampleMode = SCE_GXM_MULTISAMPLE_NONE;
		rtParams.multisampleLocations = 0;
		rtParams.driverMemBlock = -1;

		sceGxmCreateRenderTarget(&rtParams, &renderTarget);

		Log::Print(LogLevel::LEVEL_INFO, "Created render target\n");
	}

	GXMFramebuffer::~GXMFramebuffer()
	{
		gxmutils::graphicsFree(depthStencilSurface.uid);
		for (unsigned int i = 0; i < colorSurfaces.size(); ++i)
		{
			ColorSurface &cs = colorSurfaces[i];

			// clear the buffer then deallocate
			//memset(cs.data, 0, height * DISPLAY_STRIDE_IN_PIXELS * 4);
			gxmutils::graphicsFree(cs.uid);

			// destroy the sync object
			//sceGxmSyncObjectDestroy(cs.syncObj);
		}

		for (size_t i = 0; i < colorAttachments.size(); i++)
		{
			colorAttachments[i].texture->RemoveReference();
		}
		colorAttachments.clear();

		sceGxmDestroyRenderTarget(renderTarget);
	}

	void GXMFramebuffer::Resize(const FramebufferDesc &desc)
	{
	}
}
