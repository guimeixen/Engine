#pragma once

#include <string>

namespace Engine
{
	enum class TextureType
	{
		TEXTURE2D,
		TEXTURE3D,
		TEXTURE_CUBE
	};

	enum class TextureWrap
	{
		NONE,
		REPEAT,
		CLAMP,
		MIRRORED_REPEAT,
		CLAMP_TO_EDGE,
		CLAMP_TO_BORDER
	};

	enum class TextureFilter
	{
		LINEAR,
		NEAREST
	};

	enum class TextureFormat
	{
		RGB,
		RGBA,
		DEPTH_COMPONENT,
		RED,
		RG
	};

	enum class TextureInternalFormat
	{
		RGB8,
		RGBA8,
		RGB16F,
		RGBA16F,
		SRGB8,
		SRGB8_ALPHA8,
		DEPTH_COMPONENT16,
		DEPTH_COMPONENT24,
		DEPTH_COMPONENT32,
		RED8,
		R16F,
		R32UI,
		RG32UI,
		RG8
	};

	enum class TextureDataType
	{
		FLOAT,
		UNSIGNED_BYTE,
		UNSIGNED_INT
	};

	enum class ImageAccess
	{
		WRITE_ONLY,
		READ_ONLY,
		READ_WRITE,
	};

	struct TextureParams
	{
		TextureWrap wrap;
		TextureFilter filter;
		TextureFormat format;
		TextureInternalFormat internalFormat;
		TextureDataType type;
		bool useMipmapping;
		bool enableCompare;
		bool usedInCopy;
		bool usedAsStorageInCompute;
		bool usedAsStorageInGraphics;
		bool sampled;
		bool imageViewsWithDifferentFormats;
	};

	class Texture
	{
	public:
		virtual ~Texture() {}

		virtual void Bind(unsigned int slot) const = 0;
		virtual void BindAsImage(unsigned int slot, unsigned int mipLevel, bool layered, ImageAccess access, TextureInternalFormat format) const = 0;
		virtual void Unbind(unsigned int slot) const = 0;
		virtual unsigned int GetID() const = 0;
		virtual unsigned int GetWidth() const = 0;
		virtual unsigned int GetHeight() const = 0;
		virtual void Clear() = 0;

		const std::string &GetPath() const { return path; }
		unsigned char *GetData() const { return data; }
		const TextureParams &GetTextureParams() const { return params; }
		TextureType GetType() const { return type; }
		unsigned int GetMipLevels() const { return mipLevels; }

		bool IsAttachment() const { return isAttachment; }

		void AddReference() { ++refCount; }
		void RemoveReference() { if (refCount > 1) { --refCount; } else { delete this; } }
		unsigned int GetRefCount() const { return refCount; }

		static bool IsDepthTexture(TextureInternalFormat format);
		static unsigned int GetNumChannels(TextureInternalFormat format);
		static TextureInternalFormat Get4ChannelEquivalent(TextureInternalFormat format);

	protected:
		std::string path;
		bool storeTextureData;
		bool isAttachment;
		unsigned char *data;
		TextureParams params;
		TextureType type;
		unsigned int mipLevels = 1;
		unsigned int refCount = 0;
	};
}
