#pragma once

#include "Graphics\Texture.h"
#include "VKBuffer.h"

#include <vulkan\vulkan.h>

#include <string>
#include <vector>

namespace Engine
{
	class VKTextureCube : public Texture
	{
	public:
		VKTextureCube(const std::string &path, const TextureParams &params);
		VKTextureCube(const std::vector<std::string> &faces, const TextureParams &params);
		~VKTextureCube();

		void Bind(unsigned int slot) const override {}
		void BindAsImage(unsigned int slot, unsigned int mipLevel, bool layered, ImageAccess access, TextureInternalFormat format) const override;
		void Unbind(unsigned int slot) const override {}

		unsigned int GetID() const override { return 0; }
		unsigned int GetWidth() const override { return width; }
		unsigned int GetHeight() const override { return height; }

		void Clear() override;

		void Load(VKAllocator *allocator, VkPhysicalDevice physicalDevice, VkDevice device);
		void Dispose();
		void DisposeStagingBuffer();

		void CreateImageView(VkDevice device);
		void CreateSampler(VkDevice device);

		VkBuffer GetStagingBuffer() const { if (stagingBuffer) return stagingBuffer->GetBuffer(); else return VK_NULL_HANDLE; }
		VkImage GetImage() const { return image; }
		VkImageView GetImageView() const { return imageView; }
		VkSampler GetSampler() const { return sampler; }
		VkFormat GetFormat() const { return format; }
		VkImageAspectFlags GetAspectFlags() const { return aspectFlags; }

		uint32_t GetMipLevels() const { return static_cast<uint32_t>(mipLevels); }

		bool IsDepthTexture() const { return format == VK_FORMAT_D16_UNORM; }

		const std::vector<VkBufferImageCopy> &GetCopyRegions() const { return bufferCopyRegions; }

	private:
		void CreateImage(VkPhysicalDevice physicalDevice, VkDevice device);
		void LoadFaceIndividual(VkPhysicalDevice physicalDevice, VkDevice device);

	private:
		VkDevice device;
		std::vector<std::string> faces;
		uint32_t width;
		uint32_t height;
		uint32_t mipLevels;

		VKBuffer *stagingBuffer = nullptr;

		VkImage image;
		VkImageView imageView;
		VkSampler sampler;
		VkDeviceMemory imageMemory;
		VkDeviceSize size;

		VkFormat format;
		VkImageUsageFlags usageFlags;
		VkImageAspectFlags aspectFlags;
		VkSamplerAddressMode addressMode;
		VkFilter filter;
		std::vector<VkBufferImageCopy> bufferCopyRegions;
	};
}
