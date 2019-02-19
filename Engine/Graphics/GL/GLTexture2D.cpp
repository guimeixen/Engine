#include "GLTexture2D.h"

#include "GLUtils.h"
#include "Program/Log.h"

#include "include\stb_image.h"
#include "include\gli\gli.hpp"
//#include "include\zlib\zlib.h"

#include <iostream>

namespace Engine
{
	GLTexture2D::GLTexture2D(const std::string &path, const TextureParams &params, bool storeTextureData)
	{
		AddReference();
		data = nullptr;
		this->path = path;
		this->params = params;
		type = TextureType::TEXTURE2D;
		this->storeTextureData = storeTextureData;

		if (std::strstr(path.c_str(), ".ktx") > 0)
			LoadKTX();
		if (std::strstr(path.c_str(), ".dds") > 0)
			LoadDDS();
		else if (std::strstr(path.c_str(), ".raw") > 0)
			LoadRaw();
		else if ((std::strstr(path.c_str(), ".png") > 0))
			LoadPNGJPG();
		else if ((std::strstr(path.c_str(), ".jpg") > 0))
			LoadPNGJPG();
	}

	GLTexture2D::GLTexture2D(unsigned int width, unsigned int height, const TextureParams &params)
	{
		type = TextureType::TEXTURE2D;
		data = nullptr;
		this->params = params;
		this->width = width;
		this->height = height;
		this->storeTextureData = false;
		LoadWithData(nullptr);
	}

	GLTexture2D::GLTexture2D(unsigned int width, unsigned int height, const TextureParams &params, const void *data)
	{
		type = TextureType::TEXTURE2D;
		this->data = nullptr;
		this->params = params;
		this->width = width;
		this->height = height;
		this->storeTextureData = false;
		LoadWithData(data);
	}

	GLTexture2D::~GLTexture2D()
	{
		if (id > 0)
			glDeleteTextures(1, &id);

		if (data)
		{
			delete data;
			data = nullptr;
		}
	}

	void GLTexture2D::Bind(unsigned int slot) const
	{
		/*glActiveTexture(GL_TEXTURE0 + slot);
		glBindTexture(GL_TEXTURE_2D, id);*/
		glBindTextureUnit(slot, id);
	}

	void GLTexture2D::BindAsImage(unsigned int slot, unsigned int mipLevel, bool layered, ImageAccess access, TextureInternalFormat format) const
	{
		GLint glformat = glutils::InternalFormatToGL(format);

		if (access == ImageAccess::READ_WRITE)
			glBindImageTexture(slot, id, mipLevel, layered, 0, GL_READ_WRITE, glformat);
		if (access == ImageAccess::WRITE_ONLY)
			glBindImageTexture(slot, id, mipLevel, layered, 0, GL_WRITE_ONLY, glformat);
		else if (access == ImageAccess::READ_ONLY)
			glBindImageTexture(slot, id, mipLevel, layered, 0, GL_READ_ONLY, glformat);
	}

	void GLTexture2D::Unbind(unsigned int slot) const
	{
		/*glActiveTexture(GL_TEXTURE0 + slot);
		glBindTexture(GL_TEXTURE_2D, 0);*/
		glBindTextureUnit(slot, 0);
	}

	void GLTexture2D::Clear()
	{
	}

	void GLTexture2D::LoadPNGJPG()
	{
		unsigned char *image = nullptr;

		int texWidth = 0;
		int textHeight = 0;
		int channelsInFile = 0;
		int channelsInData = 0;

		if (params.format == TextureFormat::RGB)
		{
			image = stbi_load(path.c_str(), &texWidth, &textHeight, &channelsInFile, STBI_rgb);
			channelsInData = 3;
		}
		else if (params.format == TextureFormat::RGBA)
		{
			image = stbi_load(path.c_str(), &texWidth, &textHeight, &channelsInFile, STBI_rgb_alpha);
			channelsInData = 4;
		}
		else if (params.format == TextureFormat::RED)
		{
			image = stbi_load(path.c_str(), &texWidth, &textHeight, &channelsInFile, STBI_grey);
			channelsInData = 1;
		}


		if (!image)
		{
			std::cout << "ERROR -> Failed to load texture: " << path << "\n";
			return;
		}

		width = texWidth;
		height = textHeight;

		if (storeTextureData)
		{
			if (channelsInData == 1)
			{
				data = new unsigned char[width * height];
				memcpy(data, image, width * height * sizeof(unsigned char));
			}
			else if (channelsInData == 3)
			{
				data = new unsigned char[width * height * 3];
				memcpy(data, image, width * height * 3 * sizeof(unsigned char));
			}
			else if (channelsInData == 4)
			{
				data = new unsigned char[width * height * 4];
				memcpy(data, image, width * height * 4 * sizeof(unsigned char));
			}
		}

		if (params.type == TextureDataType::FLOAT)
		{
			float *h = new float[width * height];
			for (size_t i = 0; i < width * height; i++)
			{
				h[i] = (float)image[i];
			}
			LoadWithData(h);
			delete[] h;
		}
		else
		{
			LoadWithData(image);
		}
		stbi_image_free(image);
	}

	void GLTexture2D::LoadRaw()
	{
		/*float *data = nullptr;
		gzFile file = gzopen(path.c_str(), "rb");

		if (!file)
		{
			std::cout << "Error! Failed to load texture: " << path << "\n";
			return;
		}



		gzclose(file);*/
	}

	void GLTexture2D::LoadDDS()
	{
		GLsizei mipLevels;

		//std::cout << path << "\n";

		gli::texture t = gli::load(path);

		if (t.empty())
		{
			Log::Print(LogLevel::LEVEL_ERROR, "Failed to load texture: %s", path.c_str());
			path = "Data/Resources/Textures/white.dds";
			t = gli::load(path);
		}

		gli::texture2d tex(t);

		gli::gl gl(gli::gl::PROFILE_GL33);
		gli::gl::format const format = gl.translate(tex.format(), tex.swizzles());

		GLenum target = gl.translate(tex.target());

		width = tex[0].extent().x;
		height = tex[0].extent().y;
		mipLevels = static_cast<GLsizei>(tex.levels());

		if (storeTextureData)
		{
			data = new unsigned char[width * height];
			memcpy(data, tex[0].data(), width * height * sizeof(unsigned char));
		}

		glCreateTextures(target, 1, &id);

		if (params.wrap != TextureWrap::NONE)
		{
			GLint wrap = glutils::WrapToGL(params.wrap);
			glTextureParameteri(id, GL_TEXTURE_WRAP_S, wrap);
			glTextureParameteri(id, GL_TEXTURE_WRAP_T, wrap);
		}

		glTextureParameteri(id, GL_TEXTURE_MAG_FILTER, params.filter == TextureFilter::LINEAR ? GL_LINEAR : GL_NEAREST);

		if (mipLevels > 1 && params.useMipmapping)
		{
			glTextureParameteri(id, GL_TEXTURE_BASE_LEVEL, 0);
			glTextureParameteri(id, GL_TEXTURE_MAX_LEVEL, static_cast<GLint>(mipLevels - 1));
			glTextureParameteri(id, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		}
		else
			glTextureParameteri(id, GL_TEXTURE_MIN_FILTER, params.filter == TextureFilter::LINEAR ? GL_LINEAR : GL_NEAREST);

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

		if (tex.target() == gli::TARGET_2D)
			glTextureStorage2D(id, mipLevels, internalFormat, width, height);

		for (size_t level = 0; level < tex.levels(); level++)
		{
			glm::vec2 extent = glm::vec2(tex.extent(level).x, tex.extent(level).y);

			if (gli::is_compressed(tex.format()))
				glCompressedTextureSubImage2D(id, static_cast<GLint>(level), 0, 0, static_cast<GLsizei>(extent.x), static_cast<GLsizei>(extent.y), internalFormat, static_cast<GLsizei>(tex.size(level)), tex.data(0, 0, level));
			else
				glTextureSubImage2D(id, static_cast<GLint>(level), 0, 0, static_cast<GLsizei>(extent.x), static_cast<GLsizei>(extent.y), format.External, format.Type, tex.data(0, 0, level));
		}
	}

	void GLTexture2D::LoadKTX()
	{
		gli::texture t = gli::load(path);

		if (t.empty())
		{
			Log::Print(LogLevel::LEVEL_ERROR, "Failed to load texture: %s", path.c_str());
			path = "Data/Resources/Textures/white.dds";
			LoadDDS();
			return;
		}

		gli::texture2d tex2D(t);

		width = (uint32_t)tex2D.extent().x;
		height = (uint32_t)tex2D.extent().y;
		GLsizei mipLevels = static_cast<GLsizei>(tex2D.levels());

		gli::gl gl(gli::gl::PROFILE_GL33);
		gli::gl::format const format = gl.translate(tex2D.format(), tex2D.swizzles());

		glCreateTextures(GL_TEXTURE_2D, 1, &id);

		GLenum internalFormat;

		if (tex2D.format() == gli::FORMAT_RGBA_DXT5_UNORM_BLOCK16)
			internalFormat = GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT;
		else if (tex2D.format() == gli::FORMAT_RGB_DXT1_UNORM_BLOCK8)
			internalFormat = GL_COMPRESSED_SRGB_S3TC_DXT1_EXT;
		else
		{
			std::cout << "Error -> Unsupported image format!\n";
			return;
		}

		if (params.wrap != TextureWrap::NONE)
		{
			GLint wrap = glutils::WrapToGL(params.wrap);
			glTextureParameteri(id, GL_TEXTURE_WRAP_S, wrap);
			glTextureParameteri(id, GL_TEXTURE_WRAP_T, wrap);
		}

		glTextureParameteri(id, GL_TEXTURE_MAG_FILTER, params.filter == TextureFilter::LINEAR ? GL_LINEAR : GL_NEAREST);

		if (mipLevels > 1 && params.useMipmapping)
		{
			glTextureParameteri(id, GL_TEXTURE_BASE_LEVEL, 0);
			glTextureParameteri(id, GL_TEXTURE_MAX_LEVEL, static_cast<GLint>(mipLevels - 1));
			glTextureParameteri(id, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		}
		else
			glTextureParameteri(id, GL_TEXTURE_MIN_FILTER, params.filter == TextureFilter::LINEAR ? GL_LINEAR : GL_NEAREST);


		glTextureStorage2D(id, mipLevels, internalFormat, width, height);

		for (size_t level = 0; level < tex2D.levels(); level++)
		{
			glm::vec2 extent = glm::vec2(tex2D.extent(level).x, tex2D.extent(level).y);

			if (gli::is_compressed(tex2D.format()))
				glCompressedTextureSubImage2D(id, static_cast<GLint>(level), 0, 0, static_cast<GLsizei>(extent.x), static_cast<GLsizei>(extent.y), internalFormat, static_cast<GLsizei>(tex2D.size(level)), tex2D.data(0, 0, level));
			else
				glTextureSubImage2D(id, static_cast<GLint>(level), 0, 0, static_cast<GLsizei>(extent.x), static_cast<GLsizei>(extent.y), format.External, format.Type, tex2D.data(0, 0, level));
		}
	}

	void GLTexture2D::LoadWithData(const void *data)
	{
		/*glGenTextures(1, &id);
		glBindTexture(GL_TEXTURE_2D, id);*/
		glCreateTextures(GL_TEXTURE_2D, 1, &id);

		if (params.wrap != TextureWrap::NONE)
		{
			GLint wrap = glutils::WrapToGL(params.wrap);
			//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrap);
			//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrap);
			glTextureParameteri(id, GL_TEXTURE_WRAP_S, wrap);
			glTextureParameteri(id, GL_TEXTURE_WRAP_T, wrap);
		}

		//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, params.filter == TextureFilter::LINEAR ? GL_LINEAR : GL_NEAREST);
		glTextureParameteri(id, GL_TEXTURE_MAG_FILTER, params.filter == TextureFilter::LINEAR ? GL_LINEAR : GL_NEAREST);
	
		//glTexImage2D(GL_TEXTURE_2D, 0, glutils::InternalFormatToGL(params.internalFormat), width, height, 0, glutils::FormatToGL(params.format), glutils::TypeToGL(params.type), data);

		if (params.useMipmapping)
		{
			//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
			//glGenerateMipmap(GL_TEXTURE_2D);

			unsigned int i = width > height ? width : height;
			mipLevels = 0;
			while (i > 0)
			{
				mipLevels++;
				i = i >> 1;
			}

			glTextureStorage2D(id, mipLevels, glutils::InternalFormatToGL(params.internalFormat), width, height);
			if (data)
				glTextureSubImage2D(id, 0, 0, 0, width, height, glutils::FormatToGL(params.format), glutils::TypeToGL(params.type), data);

			glTextureParameteri(id, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
			glGenerateTextureMipmap(id);
		}
		else
		{
			glTextureStorage2D(id, 1, glutils::InternalFormatToGL(params.internalFormat), width, height);
			if (data)
				glTextureSubImage2D(id, 0, 0, 0, width, height, glutils::FormatToGL(params.format), glutils::TypeToGL(params.type), data);

			//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, params.filter == TextureFilter::LINEAR ? GL_LINEAR : GL_NEAREST);
			glTextureParameteri(id, GL_TEXTURE_MIN_FILTER, params.filter == TextureFilter::LINEAR ? GL_LINEAR : GL_NEAREST);
		}

		if (params.enableCompare)
		{
			//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
			//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LESS);
			glTextureParameteri(id, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
			glTextureParameteri(id, GL_TEXTURE_COMPARE_FUNC, GL_LESS);
			const GLfloat borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
			glTextureParameterfv(id, GL_TEXTURE_BORDER_COLOR, borderColor);
		}

		//glBindTexture(GL_TEXTURE_2D, 0);
	}
}
