#include "D3D11Utils.h"

namespace Engine
{
	namespace d3d11utils
	{
		DXGI_FORMAT GetFormat(TextureInternalFormat format)
		{
			switch (format)
			{
			case Engine::TextureInternalFormat::RGB8:		// Convert to rgba8 and don't use this format
				return DXGI_FORMAT_R8G8B8A8_UNORM;
				break;
			case Engine::TextureInternalFormat::RGBA8:
				return DXGI_FORMAT_R8G8B8A8_UNORM;
				break;
			case Engine::TextureInternalFormat::RGB16F:
				return DXGI_FORMAT_R16G16B16A16_FLOAT;		// Convert to rgba16 and don't use this format
				break;
			case Engine::TextureInternalFormat::RGBA16F:
				return DXGI_FORMAT_R16G16B16A16_FLOAT;
				break;
			case Engine::TextureInternalFormat::SRGB8:
				return DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;			// Convert to rgba8 and don't use this format
				break;
			case Engine::TextureInternalFormat::SRGB8_ALPHA8:
				return DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
				break;
			case Engine::TextureInternalFormat::DEPTH_COMPONENT16:
				return DXGI_FORMAT_D16_UNORM;
				break;
			case Engine::TextureInternalFormat::DEPTH_COMPONENT24:
				return DXGI_FORMAT_D24_UNORM_S8_UINT;
				break;
			case Engine::TextureInternalFormat::DEPTH_COMPONENT32:
				return DXGI_FORMAT_D32_FLOAT;
				break;
			case Engine::TextureInternalFormat::RED8:
				return DXGI_FORMAT_R8_UNORM;
				break;
			case Engine::TextureInternalFormat::R16F:
				return DXGI_FORMAT_R16_FLOAT;
				break;
			case Engine::TextureInternalFormat::R32UI:
				return DXGI_FORMAT_R32_UINT;
				break;
			}

			return DXGI_FORMAT_R8G8B8A8_UNORM;
		}

		D3D11_TEXTURE_ADDRESS_MODE GetAddressMode(TextureWrap addressMode)
		{
			switch (addressMode)
			{
			case Engine::TextureWrap::REPEAT:
				return D3D11_TEXTURE_ADDRESS_WRAP;
				break;
			case Engine::TextureWrap::CLAMP:
				return D3D11_TEXTURE_ADDRESS_CLAMP;
				break;
			case Engine::TextureWrap::MIRRORED_REPEAT:
				return D3D11_TEXTURE_ADDRESS_MIRROR;
				break;
			case Engine::TextureWrap::CLAMP_TO_EDGE:
				return D3D11_TEXTURE_ADDRESS_CLAMP;
				break;
			case Engine::TextureWrap::CLAMP_TO_BORDER:
				return D3D11_TEXTURE_ADDRESS_BORDER;
				break;
			}

			return D3D11_TEXTURE_ADDRESS_WRAP;
		}

		unsigned int GetCullMode(const std::string &mode)
		{
			if (mode == "back")
				return D3D10_CULL_BACK;
			else if (mode == "front")
				return D3D11_CULL_FRONT;

			return D3D11_CULL_BACK;
		}

		unsigned int GetFrontFace(const std::string &face)
		{
			if (face == "ccw")
				return 1;
			else if (face == "cw")
				return 0;

			return 1;
		}

		unsigned int GetDepthFunc(const std::string &func)
		{
			if (func == "less")
				return D3D11_COMPARISON_LESS;
			else if (func == "lequal")
				return D3D11_COMPARISON_LESS_EQUAL;
			else if (func == "greater")
				return D3D11_COMPARISON_GREATER;
			else if (func == "gequal")
				return D3D11_COMPARISON_GREATER_EQUAL;
			else if (func == "nequal")
				return D3D11_COMPARISON_NOT_EQUAL;
			else if (func == "equal")
				return D3D11_COMPARISON_EQUAL;
			else if (func == "never")
				return D3D11_COMPARISON_NEVER;
			else if (func == "always")
				return D3D11_COMPARISON_ALWAYS;

			return D3D11_COMPARISON_LESS;
		}

		unsigned int GetBlendFactor(BlendFactor factor)
		{
			switch (factor)
			{
			case Engine::ZERO:
				return D3D11_BLEND_ZERO;
				break;
			case Engine::ONE:
				return D3D11_BLEND_ONE;
				break;
			case Engine::SRC_ALPHA:
				return D3D11_BLEND_SRC_ALPHA;
				break;
			case Engine::DST_ALPHA:
				return D3D11_BLEND_DEST_ALPHA;
				break;
			case Engine::SRC_COLOR:
				return D3D11_BLEND_SRC_COLOR;
				break;
			case Engine::DST_COLOR:
				return D3D11_BLEND_DEST_COLOR;
				break;
			case Engine::ONE_MINUS_SRC_ALPHA:
				return D3D11_BLEND_INV_SRC_ALPHA;
				break;
			case Engine::ONE_MINUS_SRC_COLOR:
				return D3D11_BLEND_INV_SRC_COLOR;
				break;
			}

			return D3D11_BLEND_ZERO;
		}

		unsigned int GetTopology(Topology topology)
		{
			switch (topology)
			{
			case Engine::TRIANGLES:
				return D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
				break;
			case Engine::TRIANGLE_STRIP:
				return D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP;
				break;
			case Engine::LINES:
				return D3D11_PRIMITIVE_TOPOLOGY_LINELIST;
				break;
			case Engine::LINE_TRIP:
				return D3D11_PRIMITIVE_TOPOLOGY_LINESTRIP;
				break;
			}

			return D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		}
	}
}