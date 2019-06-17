#pragma once

#include "Graphics/Texture.h"

#include <psp2/gxm.h>

namespace Engine
{
	class FileManager;

	class GXMTexture2D : public Texture
	{
	public:
		GXMTexture2D();
		~GXMTexture2D();

		bool Load(FileManager *fileManager, const std::string &path, const TextureParams &params, bool storeTextureData = false);

		void Bind(unsigned int slot) const override;
		void BindAsImage(unsigned int slot, unsigned int mipLevel, bool layered, ImageAccess access, TextureInternalFormat format) const override;
		void Unbind(unsigned int slot) const override;
		unsigned int GetID() const override { return 0; }
		unsigned int GetWidth() const override { return width; }
		unsigned int GetHeight() const override { return height; }
		void Clear() override;

		const SceGxmTexture &GetGxmTexture() const { return gxmTexture; }

	private:
		unsigned int width;
		unsigned int height;
		SceUID uid;
		SceGxmTexture gxmTexture;
		SceUID paletteUID;
	};
}
