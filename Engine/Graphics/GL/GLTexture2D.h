#pragma once

#include "include\glew\glew.h"

#include "Graphics\Texture.h"

namespace Engine
{
	class GLTexture2D : public Texture
	{
	public:
		GLTexture2D(const std::string &path, const TextureParams &params, bool storeTextureData = false);
		GLTexture2D(unsigned int width, unsigned int height, const TextureParams &params);
		GLTexture2D(unsigned int width, unsigned int height, const TextureParams &params, const void* data);
		~GLTexture2D();

		void Bind(unsigned int slot) const override;
		void BindAsImage(unsigned int slot, unsigned int mipLevel, bool layered, ImageAccess access, TextureInternalFormat format) const override;
		void Unbind(unsigned int slot) const override;

		unsigned int GetID() const override { return id; }

		unsigned int GetWidth() const override { return width; }
		unsigned int GetHeight() const override { return height; }

		void Clear() override;

	private:
		void LoadPNGJPG();
		void LoadRaw();
		void LoadDDS();
		void LoadKTX();
		void LoadWithData(const void *data);

	private:
		GLuint id;
		GLuint width;
		GLuint height;
	};
}
