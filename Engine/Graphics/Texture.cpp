#include "Texture.h"

#include "Graphics\Renderer.h"
#include "Graphics\GL\GLTexture2D.h"
#include "Graphics\GL\GLTexture3D.h"
#include "Graphics\VK\VKTexture2D.h"


namespace Engine
{
	bool Texture::IsDepthTexture(TextureInternalFormat format)
	{
		switch (format)
		{	
		case Engine::TextureInternalFormat::DEPTH_COMPONENT16:
			return true;
			break;
		case Engine::TextureInternalFormat::DEPTH_COMPONENT24:
			return true;
			break;
		case Engine::TextureInternalFormat::DEPTH_COMPONENT32:
			return true;
			break;
		}

		return false;
	}
	unsigned int Texture::GetNumChannels(TextureInternalFormat format)
	{
		if (format == TextureInternalFormat::RGB8 ||
			format == TextureInternalFormat::RGB16F ||
			format == TextureInternalFormat::SRGB8)
			return 3;

		if (/*format == TextureInternalFormat::DEPTH_COMPONENT16 ||		// Check stencil
			format == TextureInternalFormat::DEPTH_COMPONENT24 ||
			format == TextureInternalFormat::DEPTH_COMPONENT32 ||*/
			format == TextureInternalFormat::R16F ||
			format == TextureInternalFormat::R32UI ||
			format == TextureInternalFormat::RED8)
			return 1;

		if (format == TextureInternalFormat::RG8)
			return 2;

		if (format == TextureInternalFormat::RGBA8 ||
			format == TextureInternalFormat::RGBA16F ||
			format == TextureInternalFormat::SRGB8_ALPHA8)
			return 4;

		return 0;
	}
	TextureInternalFormat Texture::Get4ChannelEquivalent(TextureInternalFormat format)
	{
		if (format == TextureInternalFormat::RGB8)
			return TextureInternalFormat::RGBA8;
		if (format == TextureInternalFormat::RGB16F)
			return TextureInternalFormat::RGBA16F;
		if (format == TextureInternalFormat::SRGB8)
			return TextureInternalFormat::SRGB8_ALPHA8;

		return TextureInternalFormat::RGBA8;
	}
}