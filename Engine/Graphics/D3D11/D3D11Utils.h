#pragma once

#include "Graphics\Texture.h"
#include "Graphics\MaterialInfo.h"

#include <d3d11_1.h>

namespace Engine
{
	namespace d3d11utils
	{
		DXGI_FORMAT GetFormat(TextureInternalFormat format);
		D3D11_TEXTURE_ADDRESS_MODE GetAddressMode(TextureWrap addressMode);
		unsigned int GetCullMode(const std::string &mode);
		unsigned int GetFrontFace(const std::string &face);
		unsigned int GetDepthFunc(const std::string &func);
		unsigned int GetBlendFactor(BlendFactor factor);
		unsigned int GetTopology(Topology topology);
	}
}