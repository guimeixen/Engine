#pragma once

#include "include\glew\glew.h"

#include "Graphics\Texture.h"

#include <vector>
#include <string>

namespace Engine
{
	class GLTextureCube : public Texture
	{
	public:
		GLTextureCube(const std::vector<std::string> &faces, const TextureParams &params);
		GLTextureCube(const std::string &path, const TextureParams &params);
		~GLTextureCube();

		void Bind(unsigned int slot) const override;
		void BindAsImage(unsigned int slot, unsigned int mipLevel, bool layered, ImageAccess access, TextureInternalFormat format) const override;
		void Unbind(unsigned int slot) const override;

		unsigned int GetID() const override { return id; }

		unsigned int GetWidth() const override { return width; }
		unsigned int GetHeight() const override { return height; }

		void Clear() override;

	private:
		void LoadPNGJPG();
		void LoadDDS(const std::vector<std::string> &faces);
		void LoadKTX();

	private:
		GLuint id;
		GLuint width;
		GLuint height;

		TextureParams params;
	};
}
