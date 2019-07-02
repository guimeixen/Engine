#pragma once

#include "Texture.h"

#include <vector>

namespace Engine
{
	struct FramebufferAttachment
	{
		Texture *texture;
		TextureParams params;
	};

	struct FramebufferDesc
	{
		unsigned int passID;
		unsigned int width;
		unsigned int height;
		std::vector<TextureParams> colorTextures;
		TextureParams depthTexture;
		bool sampleDepth;
		bool useDepth;
		bool writesDisabled;
	};

	class Framebuffer
	{
	public:
		virtual ~Framebuffer() {}

		unsigned int GetWidth() const { return width; }
		unsigned int GetHeight() const { return height; }

		Texture *GetColorTexture() const
		{
			if (colorAttachments.size() > 0)
				return colorAttachments[0].texture;
			return nullptr;
		}
		Texture *GetColorTextureByIndex(unsigned int index) const
		{
			if (index >= colorAttachments.size())
				return nullptr;
			return colorAttachments[index].texture;
		}

		Texture *GetDepthTexture() const { return depthAttachment.texture; }
		unsigned int GetNumColorTextures() const { return (unsigned int)colorAttachments.size(); }	
		unsigned int GetPassID() const { return passID; }

		bool AreWritesDisabled() const { return writesDisabled; }

		virtual void Resize(const FramebufferDesc &desc) = 0;
		virtual void Clear() const = 0;
		virtual void Bind() const = 0;

		void AddReference() { ++refCount; }
		void RemoveReference() { if (refCount > 1) { --refCount; } else { delete this; } }
		unsigned int GetRefCount() const { return refCount; }

	protected:
		unsigned int width;
		unsigned int height;
		unsigned int passID;
		std::vector<FramebufferAttachment> colorAttachments;
		FramebufferAttachment depthAttachment;
		bool useColor;
		bool useDepth;
		bool colorOnly;
		bool writesDisabled;
		unsigned int refCount = 0;
	};
}
