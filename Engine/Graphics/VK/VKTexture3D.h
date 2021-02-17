#pragma once

#include "Graphics/Texture.h"
#include "VKAllocator.h"

namespace Engine
{
	class VKBase;
	class VKBuffer;

	class VKTexture3D : public Texture
	{
	public:
		VKTexture3D(VKBase *base, unsigned int width, unsigned int height, unsigned int depth, const TextureParams &params, const void* data);
		~VKTexture3D();

		void Bind(unsigned int slot) const override {}
		void BindAsImage(unsigned int slot, unsigned int mipLevel, bool layered, ImageAccess access, TextureInternalFormat format) const override {}
		void Unbind(unsigned int slot) const override {}

		unsigned int GetID() const override { return id; }

		unsigned int GetWidth() const override { return width; }
		unsigned int GetHeight() const override { return height; }

		void Clear() override {}

		void Dispose();
		void DisposeStagingBuffer();		

		VkBuffer GetStagingBuffer() const;
		VkImage GetImage() const { return image; }
		VkImageView GetImageViewForAllMips() const { return imageViews[0]; }
		VkImageView GetImageViewForMip(unsigned int mip) const { return imageViews[mip + 1]; }
		VkSampler GetSampler() const { return sampler; }

		const std::vector<VkBufferImageCopy> &GetCopyRegions() const { return bufferCopyRegions; }

		uint32_t GetMipLevels() const { return static_cast<uint32_t>(mipLevels); }
		VkFormat GetFormat() const { return format; }
		VkImageAspectFlags GetAspectFlags() const { return aspectFlags; }

	private:
		void CreateImage(VkPhysicalDevice physicalDevice);
		void CreateImageView();
		void CreateSampler();

	private:
		VkDevice device;
		VKAllocator *allocator;
		uint32_t id;
		uint32_t width;
		uint32_t height;
		uint32_t depth;

		VKBuffer *stagingBuffer;

		VkImage image;
		
		std::vector<VkImageView> imageViews;
		VkDeviceSize size;
		Allocation alloc;

		VkSampler sampler;
		VkFormat format;
		VkImageUsageFlags usageFlags;
		VkImageAspectFlags aspectFlags;
		VkSamplerAddressMode addressMode;
		VkFilter filter;
		std::vector<VkBufferImageCopy> bufferCopyRegions;
	};
}
