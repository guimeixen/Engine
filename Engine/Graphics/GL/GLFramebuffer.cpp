#include "GLFramebuffer.h"

#include "GLTexture2D.h"

#include <iostream>

namespace Engine
{
	GLFramebuffer::GLFramebuffer(const FramebufferDesc &desc)
	{
		width = desc.width;
		height = desc.height;

		useColor = desc.colorTextures.size() > 0;
		useDepth = desc.useDepth;
		colorOnly = useColor && !useDepth;
		writesDisabled = desc.writesDisabled;

		if (!desc.writesDisabled)
			Create(desc);
	}

	GLFramebuffer::~GLFramebuffer()
	{
		Dispose();
	}

	void GLFramebuffer::Resize(const FramebufferDesc &desc)
	{
		if (desc.width == width && desc.height == height)
			return;

		if (writesDisabled)
			return;

		if (id > 0)
			Dispose();

		width = desc.width;
		height = desc.height;

		Create(desc);
	}

	void GLFramebuffer::Clear() const
	{
		glClear(clearMask);
	}

	void GLFramebuffer::Bind() const
	{
		glBindFramebuffer(GL_FRAMEBUFFER, id);
	}

	void GLFramebuffer::SetDefault(int width, int height, int x, int y)
	{
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glViewport(0, 0, width, height);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	}

	void GLFramebuffer::Create(const FramebufferDesc &desc)
	{
		colorAttachments.resize(desc.colorTextures.size());

		for (size_t i = 0; i < desc.colorTextures.size(); i++)
		{
			FramebufferAttachment attachment = {};
			attachment.params = desc.colorTextures[i];
			colorAttachments[i] = attachment;
		}

		depthAttachment = {};
		depthAttachment.params = desc.depthTexture;
		depthAttachment.texture = nullptr;

		glGenFramebuffers(1, &id);
		glBindFramebuffer(GL_FRAMEBUFFER, id);

		if (useColor)
		{
			std::vector<GLuint> attachments(colorAttachments.size());

			for (size_t i = 0; i < colorAttachments.size(); i++)
			{
				colorAttachments[i].texture = new GLTexture2D(width, height, colorAttachments[i].params);
				glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, GL_TEXTURE_2D, colorAttachments[i].texture->GetID(), 0);

				attachments[i] = GL_COLOR_ATTACHMENT0 + i;
			}

			glDrawBuffers(attachments.size(), attachments.data());
		}
		else
		{
			glDrawBuffer(GL_NONE);
			glReadBuffer(GL_NONE);
		}

		// Only create the depth attachment if we're not color only framebuffer
		if (desc.sampleDepth)
		{
			depthAttachment.texture = new GLTexture2D(width, height, depthAttachment.params);

			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthAttachment.texture->GetID(), 0);
		}
		else if (useDepth)
		{
			glGenRenderbuffers(1, &rboID);
			glBindRenderbuffer(GL_RENDERBUFFER, rboID);
			glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, width, height);
			glBindRenderbuffer(GL_RENDERBUFFER, 0);

			glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rboID);
		}
		GLenum fboStatus = glCheckFramebufferStatus(GL_FRAMEBUFFER);

		if (fboStatus != GL_FRAMEBUFFER_COMPLETE)
		{
			std::cout << "Framebuffer not complete: " << fboStatus << "\n";
		}
		else
		{
			if (colorOnly)
			{
				clearMask = GL_COLOR_BUFFER_BIT;
			}
			else if (useColor && useDepth)
			{
				clearMask = GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT;
			}
			else if (useDepth)
			{
				clearMask = GL_DEPTH_BUFFER_BIT;
			}
		}

		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}

	void GLFramebuffer::Dispose()
	{
		if (id > 0)
			glDeleteFramebuffers(1, &id);

		for (size_t i = 0; i < colorAttachments.size(); i++)
		{
			colorAttachments[i].texture->RemoveReference();
		}
		colorAttachments.clear();

		if (useDepth)
		{
			if (depthAttachment.texture)
			{
				depthAttachment.texture->RemoveReference();
			}
		}
		else if (rboID > 0)
			glDeleteRenderbuffers(1, &rboID);
	}
}
