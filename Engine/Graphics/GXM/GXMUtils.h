#pragma once

#include "Graphics\MaterialInfo.h"

#include "psp2\gxm.h"
#include "psp2\kernel\sysmem.h"

#include <string>

#define ALIGN(x, a) (((x) + ((a) - 1)) & ~((a) - 1))

namespace Engine
{
	namespace gxmutils
	{
		void *graphicsAlloc(SceKernelMemBlockType type, SceGxmMemoryAttribFlags gpu_attrib, size_t size, SceUID *uid);
		void graphicsFree(SceUID uid);
		void *gpuVertexUsseAllocMap(size_t size, SceUID *uid, unsigned int *usse_offset);
		void gpuVertexUsseUnmapFree(SceUID uid);
		void *gpuFragmentUsseAllocMap(size_t size, SceUID *uid, unsigned int *usse_offset);
		void gpuFragmentUsseUnmapFree(SceUID uid);

		unsigned int GetBlendFactorValue(BlendFactor blendFactor);
		unsigned int GetTopology(Topology topology);
		unsigned int GetDepthFunc(const std::string &func);
		unsigned int GetCullMode(const std::string &mode);
		unsigned int GetFrontFace(const std::string &face);
	}
}
