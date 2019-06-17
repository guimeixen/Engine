#include "GXMUtils.h"

namespace Engine
{
	namespace gxmutils
	{
		void *graphicsAlloc(SceKernelMemBlockType type, SceGxmMemoryAttribFlags gpu_attrib, size_t size, SceUID *uid)
		{
			SceUID memuid;
			void *addr;

			if (type == SCE_KERNEL_MEMBLOCK_TYPE_USER_CDRAM_RW)
				size = ALIGN(size, 256 * 1024);
			else
				size = ALIGN(size, 4 * 1024);

			memuid = sceKernelAllocMemBlock("gpumem", type, size, nullptr);
			if (memuid < 0)
				return nullptr;

			if (sceKernelGetMemBlockBase(memuid, &addr) < 0)
				return nullptr;

			if (sceGxmMapMemory(addr, size, gpu_attrib) < 0)
			{
				sceKernelFreeMemBlock(memuid);
				return nullptr;
			}

			if (uid)
				*uid = memuid;

			return addr;
		}

		void graphicsFree(SceUID uid)
		{
			void *addr;

			if (sceKernelGetMemBlockBase(uid, &addr) < 0)
				return;

			sceGxmUnmapMemory(addr);

			sceKernelFreeMemBlock(uid);
		}

		void *gpuVertexUsseAllocMap(size_t size, SceUID *uid, unsigned int *usse_offset)
		{
			SceUID memuid;
			void *addr;

			size = ALIGN(size, 4 * 1024);

			memuid = sceKernelAllocMemBlock("gpu_vertex_usse", SCE_KERNEL_MEMBLOCK_TYPE_USER_RW_UNCACHE, size, nullptr);
			if (memuid < 0)
				return nullptr;

			if (sceKernelGetMemBlockBase(memuid, &addr) < 0)
				return nullptr;

			if (sceGxmMapVertexUsseMemory(addr, size, usse_offset) < 0)
				return nullptr;

			return addr;
		}

		void gpuVertexUsseUnmapFree(SceUID uid)
		{
			void *addr;

			if (sceKernelGetMemBlockBase(uid, &addr) < 0)
				return;

			sceGxmUnmapVertexUsseMemory(addr);

			sceKernelFreeMemBlock(uid);
		}

		void *gpuFragmentUsseAllocMap(size_t size, SceUID *uid, unsigned int *usse_offset)
		{
			SceUID memuid;
			void *addr;

			size = ALIGN(size, 4 * 1024);

			memuid = sceKernelAllocMemBlock("gpu_fragment_usse", SCE_KERNEL_MEMBLOCK_TYPE_USER_RW_UNCACHE, size, nullptr);
			if (memuid < 0)
				return nullptr;

			if (sceKernelGetMemBlockBase(memuid, &addr) < 0)
				return nullptr;

			if (sceGxmMapFragmentUsseMemory(addr, size, usse_offset) < 0)
				return nullptr;

			return addr;
		}

		void gpuFragmentUsseUnmapFree(SceUID uid)
		{
			void *addr;

			if (sceKernelGetMemBlockBase(uid, &addr) < 0)
				return;

			sceGxmUnmapFragmentUsseMemory(addr);

			sceKernelFreeMemBlock(uid);
		}

		unsigned int GetBlendFactorValue(BlendFactor blendFactor)
		{
			/*switch (blendFactor)
			{
			case ZERO:
				return VK_BLEND_FACTOR_ZERO;
				break;
			case ONE:
				return VK_BLEND_FACTOR_ONE;
				break;
			case SRC_ALPHA:
				return VK_BLEND_FACTOR_SRC_ALPHA;
				break;
			case DST_ALPHA:
				return VK_BLEND_FACTOR_DST_ALPHA;
				break;
			case SRC_COLOR:
				return VK_BLEND_FACTOR_SRC_COLOR;
				break;
			case DST_COLOR:
				return VK_BLEND_FACTOR_DST_COLOR;
				break;
			case ONE_MINUS_SRC_ALPHA:
				return VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
				break;
			case ONE_MINUS_SRC_COLOR:
				return VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR;
				break;
			}

			return VK_BLEND_FACTOR_ZERO;*/

			return 0;
		}

		unsigned int GetTopology(Topology topology)
		{
			/*switch (topology)
			{
			case TRIANGLES:
				return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
				break;
			case TRIANGLE_STRIP:
				return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
				break;
			case LINES:
				return VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
				break;
			case LINE_TRIP:
				return VK_PRIMITIVE_TOPOLOGY_LINE_STRIP;
				break;
			default:
				break;
			}

			return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;*/

			return 0;
		}

		unsigned int GetDepthFunc(const std::string &func)
		{
			/*if (func == "less")
				return VK_COMPARE_OP_LESS;
			else if (func == "lequal")
				return VK_COMPARE_OP_LESS_OR_EQUAL;
			else if (func == "greater")
				return VK_COMPARE_OP_GREATER;
			else if (func == "gequal")
				return VK_COMPARE_OP_GREATER_OR_EQUAL;
			else if (func == "nequal")
				return VK_COMPARE_OP_NOT_EQUAL;
			else if (func == "equal")
				return VK_COMPARE_OP_EQUAL;
			else if (func == "never")
				return VK_COMPARE_OP_NEVER;
			else if (func == "always")
				return VK_COMPARE_OP_ALWAYS;

			return VK_COMPARE_OP_LESS;*/

			return 0;
		}

		unsigned int GetCullMode(const std::string &mode)
		{
			/*if (mode == "front")
				return VK_CULL_MODE_FRONT_BIT;
			else if (mode == "back")
				return VK_CULL_MODE_BACK_BIT;
			else if (mode == "none")
				return SCE_GXM_CULL_NONE;

			return VK_CULL_MODE_BACK_BIT;*/
			return 0;
		}

		unsigned int GetFrontFace(const std::string &face)
		{
			if (face == "ccw")
				return SCE_GXM_CULL_CCW;
			else if (face == "cw")
				return SCE_GXM_CULL_CW;

			return SCE_GXM_CULL_CCW;
		}
	}
}
