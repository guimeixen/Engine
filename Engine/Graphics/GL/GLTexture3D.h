#pragma once

#include "Graphics\Texture.h"

namespace Engine
{
	class GLTexture3D : public Texture
	{
	public:
		GLTexture3D(unsigned int width, unsigned int height, unsigned int depth, const TextureParams &params, const void* data);
		~GLTexture3D();

		void Bind(unsigned int slot) const override;
		void BindAsImage(unsigned int slot, unsigned int mipLevel, bool layered, ImageAccess access, TextureInternalFormat format) const override;
		void Unbind(unsigned int slot) const override;

		unsigned int GetID() const override { return id; }

		unsigned int GetWidth() const override { return width; }
		unsigned int GetHeight() const override { return height; }

		void Clear() override;

	private:
		void LoadWithData(const void *data);

	private:
		unsigned int id;
		unsigned int width;
		unsigned int height;
		unsigned int depth;
		int format;
	};
}
