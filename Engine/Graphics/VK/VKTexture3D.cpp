#include "VKTexture3D.h"

#include "VKBase.h"
#include "Program/Log.h"
#include "include/half.hpp"

#include <iostream>

namespace Engine
{
	VKTexture3D::VKTexture3D(VKBase *context, unsigned int width, unsigned int height, unsigned int depth, const TextureParams &params, const void *data)
	{
		AddReference();
		allocator = context->GetAllocator();
		this->data = nullptr;
		this->params = params;
		this->width = width;
		this->height = height;
		this->depth = depth;
		storeTextureData = false;
		device = context->GetDevice();
		isAttachment = false;
		type = TextureType::TEXTURE3D;
		aspectFlags = VK_IMAGE_ASPECT_COLOR_BIT;
		sampler = VK_NULL_HANDLE;
		addressMode = vkutils::GetAddressMode(params.wrap);
		filter = vkutils::GetFilter(params.filter);
		format = vkutils::GetFormat(params.internalFormat);

		if (params.useMipmapping)
		{
			mipLevels = (size_t)std::floor(std::log2(float(std::min(width, height)))) + 1;
			imageViews.resize(mipLevels + 1);
		}
		else
		{
			mipLevels = 1;
			imageViews.resize(1);
		}

		
		unsigned int numChannels = Texture::GetNumChannels(params.internalFormat);
		if (params.type == TextureDataType::FLOAT)
		{
			size = width * height * depth * numChannels * sizeof(float);
		}
		else if (params.type == TextureDataType::UNSIGNED_BYTE)
		{
			size = width * height * depth * numChannels * sizeof(unsigned char);
		}

		if (params.usedAsStorageInCompute || params.usedAsStorageInGraphics)
		{
			VkFormatProperties formatProps = {};
			vkGetPhysicalDeviceFormatProperties(context->GetPhysicalDevice(), format, &formatProps);
			if ((formatProps.optimalTilingFeatures & VK_FORMAT_FEATURE_STORAGE_IMAGE_BIT) == false)
			{
				std::cout << "Missing support for storage image bit\n";
				return;
			}
	
			if (params.sampled)
				usageFlags = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;		// Image clear acts as a tranfer operation so we need to use transfer_dst
			else
				usageFlags  = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_STORAGE_BIT;
		}
		else if (params.usedInCopy)
		{
			usageFlags = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
		}
		else
		{
			usageFlags = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;			// The image is going to be used as a dst for a buffer copy and we will also access it from the shader
		}



		if (data)
		{
			// Because Vulkan doesn't support formats with 24 bits we convert to 32 bits
			if (numChannels == 3 && params.type == TextureDataType::FLOAT)
			{
				// copy to 32 bits
			}
			else if (numChannels == 4 && params.type == TextureDataType::FLOAT)
			{
				half_float::half *newData = new half_float::half[width * height * depth * 4];
				const float *oldData = static_cast<const float*>(data);
				unsigned int counter = 0;
				unsigned int counterOld = 0;

				size = width * height * depth * 4 * sizeof(half_float::half);

				// Better way to copy?
				for (unsigned int i = 0; i < width * height * depth; i++)
				{
					newData[counter++] = half_float::half_cast<half_float::half>(oldData[counterOld++]);
					newData[counter++] = half_float::half_cast<half_float::half>(oldData[counterOld++]);
					newData[counter++] = half_float::half_cast<half_float::half>(oldData[counterOld++]);
					newData[counter++] = half_float::half_cast<half_float::half>(oldData[counterOld++]);
				}

				stagingBuffer = new VKBuffer();
				stagingBuffer->Create(allocator, context->GetPhysicalDevice(), device, size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, false);
				stagingBuffer->Map();
				stagingBuffer->Update(newData, (unsigned int)size, 0);
				stagingBuffer->Unmap();

				delete[] newData;
			}
			else if ((numChannels == 1 || numChannels == 2 || numChannels == 4) && params.type == TextureDataType::UNSIGNED_BYTE)
			{
				// No need to modify, copy straight to staging buffer
				stagingBuffer = new VKBuffer();
				stagingBuffer->Create(allocator, context->GetPhysicalDevice(), device, size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, false);
				stagingBuffer->Map();
				stagingBuffer->Update(data, (unsigned int)size, 0);
				stagingBuffer->Unmap();
			}

			// Setup buffer copy region
			VkBufferImageCopy region = {};
			region.imageSubresource.aspectMask = aspectFlags;
			region.imageSubresource.mipLevel = 0;
			region.imageSubresource.baseArrayLayer = 0;
			region.imageSubresource.layerCount = 1;
			region.imageExtent.width = static_cast<uint32_t>(width);
			region.imageExtent.height = static_cast<uint32_t>(height);
			region.imageExtent.depth = static_cast<uint32_t>(depth);
			region.bufferOffset = 0;

			bufferCopyRegions.push_back(region);
		}

		CreateImage(context->GetPhysicalDevice());

		if ((!params.usedAsStorageInCompute && !params.usedAsStorageInGraphics) || params.sampled)
			CreateSampler();

		CreateImageView();	
	}

	VKTexture3D::~VKTexture3D()
	{
		if (device)
			Dispose();

		DisposeStagingBuffer();
	}

	void VKTexture3D::Dispose()
	{
		if (sampler != VK_NULL_HANDLE)
		{
			vkDestroySampler(device, sampler, nullptr);
		}
		for (size_t i = 0; i < imageViews.size(); i++)
		{
			vkDestroyImageView(device, imageViews[i], nullptr);
		}
		vkDestroyImage(device, image, nullptr);

		allocator->Free(alloc);
	}

	void VKTexture3D::DisposeStagingBuffer()
	{
		if (stagingBuffer)
		{
			delete stagingBuffer;
			stagingBuffer = nullptr;
		}
	}

	void VKTexture3D::CreateImage(VkPhysicalDevice physicalDevice)
	{
		VkImageCreateInfo imageInfo = {};
		imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		imageInfo.arrayLayers = 1;
		imageInfo.extent.width = width;
		imageInfo.extent.height = height;
		imageInfo.extent.depth = depth;
		imageInfo.imageType = VK_IMAGE_TYPE_3D;
		imageInfo.mipLevels = mipLevels;
		imageInfo.usage = usageFlags;
		imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
		imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		imageInfo.flags = 0;
		imageInfo.format = format;
		imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
		imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

		if (vkCreateImage(device, &imageInfo, nullptr, &image) != VK_SUCCESS)
		{
			std::cout << "Error -> Failed to create image!\n";
		}

		VkMemoryRequirements memReqs;
		vkGetImageMemoryRequirements(device, image, &memReqs);

		allocator->Allocate(alloc, memReqs.size, vkutils::FindMemoryType(physicalDevice, memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT), false);

		vkBindImageMemory(device, image, alloc.memory, alloc.offset);
	}

	void VKTexture3D::CreateImageView()
	{
		// Textures are not directly accessed by the shaders and
		// are abstracted by image views containing additional
		// information and sub resource ranges
		VkImageViewCreateInfo viewInfo = {};
		viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		viewInfo.image = image;
		viewInfo.viewType = VK_IMAGE_VIEW_TYPE_3D;
		viewInfo.format = format;
		viewInfo.components = { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A };
		viewInfo.subresourceRange.aspectMask = aspectFlags;
		viewInfo.subresourceRange.baseArrayLayer = 0;
		viewInfo.subresourceRange.layerCount = 1;

		if (params.useMipmapping)
		{
			// We create mipLevels + 1 imageViews. The first can access all mips, the second can access the first mip, the third the second and so on
			for (size_t i = 0; i < mipLevels + 1; i++)
			{
				// First view can access every mip. Remaining views can only access one mip 
				if (i == 0)
				{					
					viewInfo.subresourceRange.baseMipLevel = 0;
					viewInfo.subresourceRange.levelCount = static_cast<uint32_t>(mipLevels);				
				}
				else
				{
					viewInfo.subresourceRange.baseMipLevel = static_cast<uint32_t>(i - 1);
					viewInfo.subresourceRange.levelCount = 1;
				}

				if (vkCreateImageView(device, &viewInfo, nullptr, &imageViews[i]) != VK_SUCCESS)
					Log::Print(LogLevel::LEVEL_ERROR, "Error -> Failed to create image view for mip %i!\n", i);
			}		
		}
		else
		{
			// The subresource range describes the set of mip levels (and array layers) that can be accessed through this image view
			// It's possible to create multiple image views for a single image referring to different (and/or overlapping) ranges of the image
			viewInfo.subresourceRange.aspectMask = aspectFlags;
			viewInfo.subresourceRange.baseMipLevel = 0;
			viewInfo.subresourceRange.levelCount = static_cast<uint32_t>(mipLevels);
			viewInfo.subresourceRange.baseArrayLayer = 0;
			viewInfo.subresourceRange.layerCount = 1;

			if (vkCreateImageView(device, &viewInfo, nullptr, &imageViews[0]) != VK_SUCCESS)
				Log::Print(LogLevel::LEVEL_ERROR, "Error -> Failed to create image view!\n");
		}
	}

	void VKTexture3D::CreateSampler()
	{
		// Here we're using a sampler per texture but they can be applied to any texture we want. So we could create the necessary samplers and then reuse them
		VkSamplerCreateInfo samplerInfo = {};
		samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		samplerInfo.magFilter = filter;
		samplerInfo.minFilter = filter;
		samplerInfo.addressModeU = addressMode;
		samplerInfo.addressModeV = addressMode;
		samplerInfo.addressModeW = addressMode;
		samplerInfo.anisotropyEnable = VK_FALSE;
		samplerInfo.maxAnisotropy = 1.0f;
		samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
		samplerInfo.unnormalizedCoordinates = VK_FALSE;
		samplerInfo.compareEnable = VK_FALSE;
		samplerInfo.compareOp = VK_COMPARE_OP_NEVER;
		samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
		samplerInfo.mipLodBias = 0.0f;
		samplerInfo.minLod = 0.0f;
		samplerInfo.maxLod = (float)mipLevels;

		if (vkCreateSampler(device, &samplerInfo, nullptr, &sampler) != VK_SUCCESS)
		{
			std::cout << "Error -> Failed to create sampler!\n";
		}
	}
	
}
