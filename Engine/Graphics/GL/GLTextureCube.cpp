#include "GLTextureCube.h"

#include "GLUtils.h"

#include "include\gli\gli.hpp"

#include <iostream>

namespace Engine
{
	GLTextureCube::GLTextureCube(const std::vector<std::string> &faces, const TextureParams &params)
	{
		this->params = params;
		this->type = TextureType::TEXTURE_CUBE;

		if (std::strstr(faces[0].c_str(), ".dds") > 0)
			LoadDDS(faces);
		else
			LoadPNGJPG();
	}

	GLTextureCube::GLTextureCube(const std::string &path, const TextureParams &params)
	{
		this->path = path;
		this->params = params;
		this->type = TextureType::TEXTURE_CUBE;

		if (std::strstr(path.c_str(), ".ktx") > 0)
			LoadKTX();
	}

	GLTextureCube::~GLTextureCube()
	{
		if (id > 0)
			glDeleteTextures(1, &id);
	}

	void GLTextureCube::Bind(unsigned int slot) const
	{
		//glActiveTexture(GL_TEXTURE0 + slot);
		//glBindTexture(GL_TEXTURE_CUBE_MAP, id);
		glBindTextureUnit(slot, id);
	}

	void GLTextureCube::BindAsImage(unsigned int slot, unsigned int mipLevel, bool layered, ImageAccess access, TextureInternalFormat format) const
	{
	}

	void GLTextureCube::Unbind(unsigned int slot) const
	{
		//glActiveTexture(GL_TEXTURE0 + slot);
		//glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
		glBindTextureUnit(slot, 0);
	}

	void GLTextureCube::Clear()
	{
	}

	void GLTextureCube::LoadPNGJPG()
	{
	}

	void GLTextureCube::LoadDDS(const std::vector<std::string> &faces)
	{
		//GLsizei mipLevels;

		glGenTextures(1, &id);
		glBindTexture(GL_TEXTURE_CUBE_MAP, id);

		for (size_t i = 0; i < faces.size(); i++)
		{
			gli::texture2d tex(gli::load(faces[i]));

			if (tex.empty())
			{
				std::cout << "ERROR -> Failed to load texture: " << faces[i] << "\n";
				glDeleteTextures(1, &id);		// Delete because we generated it above
				return;
			}

			gli::gl gl(gli::gl::PROFILE_GL33);
			gli::gl::format const format = gl.translate(tex.format(), tex.swizzles());

			width = tex[0].extent().x;
			height = tex[0].extent().y;

			//mipLevels = static_cast<GLsizei>(tex.levels());

			/*if (mipLevels > 1 && params.useMipmapping)
			{
				glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_BASE_LEVEL, 0);
				glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAX_LEVEL, static_cast<GLint>(mipLevels - 1));
				glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
			}
			else*/
			glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, params.filter == TextureFilter::LINEAR ? GL_LINEAR : GL_NEAREST);

			GLenum internalFormat;
			if (format.Internal == gli::gl::INTERNAL_RGBA_DXT1)
			{
				internalFormat = GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT1_EXT;
			}
			else if (format.Internal == gli::gl::INTERNAL_RGBA_DXT3)
			{
				internalFormat = GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT3_EXT;
			}
			else if (format.Internal == gli::gl::INTERNAL_RGBA_DXT5)
			{
				internalFormat = GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT;
			}

			glCompressedTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, internalFormat, width, height, 0, tex.size(0), tex[0].data());

			/*for (size_t level = 0; level < tex.levels(); level++)
			{
				glm::vec2 extent = glm::vec2(tex.extent(level).x, tex.extent(level).y);

				if (gli::is_compressed(tex.format()))
					glCompressedTexSubImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, static_cast<GLint>(level), 0, 0, static_cast<GLsizei>(extent.x), static_cast<GLsizei>(extent.y), internalFormat, static_cast<GLsizei>(tex.size(level)), tex.data(0, 0, level));
				else
					glTexSubImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, static_cast<GLint>(level), 0, 0, extent.x, extent.y, format.External, format.Type, tex.data(0, 0, level));
			}*/
		}

		if (params.wrap != TextureWrap::NONE)
		{
			GLint wrap = glutils::WrapToGL(params.wrap);
			glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, wrap);
			glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, wrap);
			glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, wrap);
		}

		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, params.filter == TextureFilter::LINEAR ? GL_LINEAR : GL_NEAREST);

		glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
	}

	void GLTextureCube::LoadKTX()
	{
		gli::texture_cube texCube(gli::load(path));
		if (texCube.empty())
		{
			std::cout << "Error -> Failed to load texture:" << path << '\n';
			return;
		}

		width = (uint32_t)texCube.extent().x;
		height = (uint32_t)texCube.extent().y;

		glCreateTextures(GL_TEXTURE_CUBE_MAP, 1, &id);

		GLenum internalFormat;

		if (texCube.format() == gli::FORMAT_RGBA_DXT5_UNORM_BLOCK16)
			internalFormat = GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT;
		else
		{
			std::cout << "Error -> Unsupported image format!\n";
			return;
		}

		glTextureParameteri(id, GL_TEXTURE_MIN_FILTER, params.filter == TextureFilter::LINEAR ? GL_LINEAR : GL_NEAREST);
		glTextureParameteri(id, GL_TEXTURE_MAG_FILTER, params.filter == TextureFilter::LINEAR ? GL_LINEAR : GL_NEAREST);

		if (params.wrap != TextureWrap::NONE)
		{
			GLint wrap = glutils::WrapToGL(params.wrap);
			glTextureParameteri(id, GL_TEXTURE_WRAP_S, wrap);
			glTextureParameteri(id, GL_TEXTURE_WRAP_T, wrap);
			glTextureParameteri(id, GL_TEXTURE_WRAP_R, wrap);
		}

		glTextureStorage2D(id, 1, internalFormat, width, height);

		for (size_t i = 0; i < 6; i++)
		{
			// i = the offset to desired cubemap face
			// depth = how many faces to set, if this was 3 we'd set 3 cubemap faces at once
			glCompressedTextureSubImage3D(id, 0, 0, 0, i, width, height, 1, internalFormat, texCube[i].size(0), texCube[i][0].data());
			//glCompressedTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, ,, internalFormat, width, height, 0, texCube.size(0), texCube[i].data());
		}
	}
}
