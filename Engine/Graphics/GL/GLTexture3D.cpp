#include "GLTexture3D.h"

#include "GLUtils.h"

namespace Engine
{
	GLTexture3D::GLTexture3D(unsigned int width, unsigned int height, unsigned int depth, const TextureParams &params, const void *data)
	{
		this->data = nullptr;
		this->params = params;
		this->width = width;
		this->height = height;
		this->depth = depth;
		this->storeTextureData = false;
		type = TextureType::TEXTURE3D;
		LoadWithData(data);
	}

	GLTexture3D::~GLTexture3D()
	{
		if (id)
			glDeleteTextures(1, &id);
	}

	void GLTexture3D::Bind(unsigned int slot) const
	{
		glBindTextureUnit(slot, id);
	}

	void GLTexture3D::BindAsImage(unsigned int slot, unsigned int mipLevel, bool layered, ImageAccess access, TextureInternalFormat format) const
	{
		GLint glformat = glutils::InternalFormatToGL(format);

		if (access == ImageAccess::READ_WRITE)
			glBindImageTexture(slot, id, mipLevel, layered, 0, GL_READ_WRITE, glformat);
		if (access == ImageAccess::WRITE_ONLY)
			glBindImageTexture(slot, id, mipLevel, layered, 0, GL_WRITE_ONLY, glformat);
		else if(access == ImageAccess::READ_ONLY)
			glBindImageTexture(slot, id, mipLevel, layered, 0, GL_READ_ONLY, glformat);
	}

	void GLTexture3D::Unbind(unsigned int slot) const
	{
		glBindTextureUnit(slot, 0);
	}

	void GLTexture3D::Clear()
	{
		unsigned char clearColor[4] = { 0,0,0,0 };
		glClearTexImage(id, 0, GL_RGBA, GL_UNSIGNED_BYTE, clearColor);
	}

	void GLTexture3D::LoadWithData(const void *data)
	{
		glCreateTextures(GL_TEXTURE_3D, 1, &id);

		if (params.wrap != TextureWrap::NONE)
		{
			GLint wrap = glutils::WrapToGL(params.wrap);
			glTextureParameteri(id, GL_TEXTURE_WRAP_S, wrap);
			glTextureParameteri(id, GL_TEXTURE_WRAP_T, wrap);
			glTextureParameteri(id, GL_TEXTURE_WRAP_R, wrap);
		}

		glTextureParameteri(id, GL_TEXTURE_MAG_FILTER, params.filter == TextureFilter::LINEAR ? GL_LINEAR : GL_NEAREST);

		// No mipmapping support yet for textures with data

		// If the texture is power of two and we don't provide data then we can set the mipmap levels

		format = glutils::InternalFormatToGL(params.internalFormat);

		if (width == height && width == depth && !data && params.useMipmapping)
		{
			int i = width;
			unsigned int mipLevels = 0;
			while (i > 0)
			{
				mipLevels++;
				i = i >> 1;
			}

			this->mipLevels = mipLevels;

			glTextureStorage3D(id, mipLevels, format, width, height, depth);

			unsigned char clearColor[4] = { 0,0,0,0 };
			
			for (unsigned int i = 0; i < mipLevels; i++)
			{
				glClearTexImage(id, i, GL_RGBA, GL_UNSIGNED_BYTE, &clearColor);
			}
		}
		else
		{
			glTextureStorage3D(id, 1, format, width, height, depth);
			if (data)
				glTextureSubImage3D(id, 0, 0, 0, 0, width, height, depth, glutils::FormatToGL(params.format), glutils::TypeToGL(params.type), data);
		}
		
		if (params.useMipmapping)
		{
			glTextureParameteri(id, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
			// implement the rest
		}
		else
		{
			glTextureParameteri(id, GL_TEXTURE_MIN_FILTER, params.filter == TextureFilter::LINEAR ? GL_LINEAR : GL_NEAREST);
		}
	}
}
