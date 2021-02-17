#pragma once

#include "Graphics/Texture.h"
#include "VKAllocator.h"

namespace Engine
{
	class VKBase;
	class VKBuffer;

	class VKTexture2D : public Texture
	{
	public:
		VKTexture2D();
		VKTexture2D(VKBase *base, unsigned int width, unsigned int height, const TextureParams &params, const void *data);
		VKTexture2D(VKBase *base, const std::string &path, const TextureParams &params, bool storeTextureData);
		~VKTexture2D();

		void Bind(unsigned int slot) const override;
		void BindAsImage(unsigned int slot, unsigned int mipLevel, bool layered, ImageAccess access, TextureInternalFormat format) const override;
		void Unbind(unsigned int slot) const override;

		unsigned int GetID() const override { return id; }
		unsigned int GetWidth() const override { return width; }
		unsigned int GetHeight() const override { return height; }

		void Clear() override;

		void Load(VKBase* base);

		void CreateColorAttachment(VKAllocator *allocator, VkPhysicalDevice physicalDevice, VkDevice device, uint32_t width, uint32_t height, const TextureParams &params);
		void CreateDepthStencilAttachment(VKAllocator *allocator, VkPhysicalDevice physicalDevice, VkDevice device, uint32_t width, uint32_t height, const TextureParams &params, bool useInShader, bool useStencil);

		void Dispose();
		void DisposeStagingBuffer();

		void CreateImageView();
		void CreateSampler();

		VkBuffer GetStagingBuffer() const;
		VkImage GetImage() const { return image; }
		VkImageView GetImageView() const { return imageView; }
		VkSampler GetSampler() const { return sampler; }

		const std::vector<VkBufferImageCopy> &GetCopyRegions() const { return bufferCopyRegions; }

		uint32_t GetMipLevels() const { return static_cast<uint32_t>(mipLevels); }
		VkFormat GetFormat() const { return format; }
		VkImageAspectFlags GetAspectFlags() const { return aspectFlags; }
		bool IsDepthTexture() const { return format == VK_FORMAT_D16_UNORM || format == VK_FORMAT_D24_UNORM_S8_UINT || format == VK_FORMAT_D32_SFLOAT; }
		bool WantsMipmaps() const { return params.useMipmapping; }
		bool HasMipmaps() const { return mipmapsGenerated; }

	private:
		void CreateImage(VkPhysicalDevice physicalDevice);
		void LoadPNGJPG(VKBase* base);

	private:
		VkDevice device;
		VKAllocator *allocator;
		uint32_t id;
		uint32_t width;
		uint32_t height;

		VKBuffer *stagingBuffer;

		VkImage image;
		VkImageView imageView;
		VkDeviceSize size;

		Allocation alloc;

		VkSampler sampler;
		VkFormat format;
		VkImageUsageFlags usageFlags;
		VkImageAspectFlags aspectFlags;
		VkSamplerAddressMode addressMode;
		VkFilter filter;
		size_t mipLevels;
		std::vector<VkBufferImageCopy> bufferCopyRegions;
		bool mipmapsGenerated;
	};
}
