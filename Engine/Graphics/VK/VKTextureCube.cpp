#include "VKTextureCube.h"

#include "VKUtils.h"

#include "include\gli\gli.hpp"

#include <iostream>

namespace Engine
{
	VKTextureCube::VKTextureCube(const std::string &path, const TextureParams &params)
	{
		width = 0;
		height = 0;
		this->path = path;
		this->type = TextureType::TEXTURE_CUBE;
		faces.push_back(path);
		this->params = params;

		filter = vkutils::GetFilter(params.filter);
		addressMode = vkutils::GetAddressMode(params.wrap);

		usageFlags = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;			// The image is going to be used as a dst for a buffer copy and we will also access it from the shader
		aspectFlags = VK_IMAGE_ASPECT_COLOR_BIT;
		sampler = VK_NULL_HANDLE;
	}

	VKTextureCube::VKTextureCube(const std::vector<std::string> &faces, const TextureParams &params)
	{
		this->faces = faces;
		width = 0;
		height = 0;
		this->params = params;
		this->type = TextureType::TEXTURE_CUBE;

		filter = vkutils::GetFilter(params.filter);
		addressMode = vkutils::GetAddressMode(params.wrap);

		usageFlags = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;			// The image is going to be used as a dst for a buffer copy and we will also access it from the shader
		aspectFlags = VK_IMAGE_ASPECT_COLOR_BIT;
		sampler = VK_NULL_HANDLE;
	}

	VKTextureCube::~VKTextureCube()
	{
		if (device)
			Dispose();

		DisposeStagingBuffer();
	}

	void VKTextureCube::BindAsImage(unsigned int slot, unsigned int mipLevel, bool layered, ImageAccess access, TextureInternalFormat format) const
	{
	}

	void VKTextureCube::Clear()
	{
	}

	void VKTextureCube::Load(VKAllocator *allocator, VkPhysicalDevice physicalDevice, VkDevice device)
	{
		this->device = device;

		// Check if we're going to load the 6 faces individually or all at once
		if (faces.size() == 6)
		{
			for (int i = 0; i < 6; i++)
			{
				gli::texture2d face(gli::load(faces[i]));
				if (face.empty())
				{
					std::cout << "Error -> Failed to load texture!\n";
					return;
				}


			}
		}
		else if (faces.size() == 1)
		{
			gli::texture_cube texCube(gli::load(path));
			if (texCube.empty())
			{
				std::cout << "Error -> Failed to load texture!\n";
				return;
			}

			if (texCube.format() == gli::FORMAT_RGBA_DXT5_UNORM_BLOCK16)
				format = VK_FORMAT_BC3_UNORM_BLOCK;
			else
			{
				std::cout << "Error -> Unsupported image format!\n";
				return;
			}

			width = (uint32_t)texCube.extent().x;
			height = (uint32_t)texCube.extent().y;
			mipLevels = (uint32_t)texCube.levels();
			size = (uint32_t)texCube.size();

			usageFlags = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;			// The image is going to be used as a dst for a buffer copy and we will also access it from the shader
			aspectFlags = VK_IMAGE_ASPECT_COLOR_BIT;

			stagingBuffer = new VKBuffer();
			stagingBuffer->Create(allocator, physicalDevice, device, size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, false);
			stagingBuffer->Map();
			stagingBuffer->Update(texCube.data(), (unsigned int)size, 0);
			stagingBuffer->Unmap();

			// Setup buffer copy regions for wach mip map level
			uint32_t offset = 0;

			for (uint32_t face = 0; face < 6; face++)
			{
				for (uint32_t i = 0; i < mipLevels; i++)
				{
					VkBufferImageCopy region = {};
					region.imageSubresource.aspectMask = aspectFlags;
					region.imageSubresource.mipLevel = i;
					region.imageSubresource.layerCount = 1;
					region.imageSubresource.baseArrayLayer = face;
					region.imageExtent.width = static_cast<uint32_t>(texCube[face][i].extent().x);
					region.imageExtent.height = static_cast<uint32_t>(texCube[face][i].extent().y);
					region.imageExtent.depth = 1;
					region.bufferOffset = offset;

					bufferCopyRegions.push_back(region);

					offset += static_cast<uint32_t>(texCube[face][i].size());
				}
			}
		}

		CreateImage(physicalDevice, device);
	}

	void VKTextureCube::Dispose()
	{
		if (sampler != VK_NULL_HANDLE)
		{
			vkDestroySampler(device, sampler, nullptr);
		}
		vkDestroyImageView(device, imageView, nullptr);		// Destroy image view before the image
		vkDestroyImage(device, image, nullptr);
		vkFreeMemory(device, imageMemory, nullptr);
	}

	void VKTextureCube::DisposeStagingBuffer()
	{
		if (stagingBuffer)
		{
			delete stagingBuffer;
			stagingBuffer = nullptr;
		}
	}

	void VKTextureCube::CreateImage(VkPhysicalDevice physicalDevice, VkDevice device)
	{
		VkImageCreateInfo imageInfo = {};
		imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;

		imageInfo.extent.width = width;
		imageInfo.extent.height = height;
		imageInfo.extent.depth = 1;
		imageInfo.imageType = VK_IMAGE_TYPE_2D;
		imageInfo.mipLevels = mipLevels;
		imageInfo.usage = usageFlags;
		imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
		imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		imageInfo.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;		// Required for cube map images
		imageInfo.arrayLayers = 6;									// Cube faces count as array layers in Vulkan
		imageInfo.format = format;			// We should use the same format as the pixels in the buffer above
		imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;				// Texesl are laid out in an implementation defined order for optimal access. More efficient when accessing in the shader
																// Tiling mode cannot b changed at a later time. To directly access texles in the memory of the image, then we must use VK_IMAGE_TILING_LINEAR
																// We will be using a staging buffer instead of a staging image, so this won't be necessary

		imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;		//  LAYOUT_UNDEFINED if we were using it as a color attachment which would probably be cleared anyway
																	// PREINITIALIZED will preserve the texels on the first transition

		if (vkCreateImage(device, &imageInfo, nullptr, &image) != VK_SUCCESS)
		{
			std::cout << "Error -> Failed to create image!\n";
		}

		VkMemoryRequirements memReqs;
		vkGetImageMemoryRequirements(device, image, &memReqs);

		VkMemoryAllocateInfo allocInfo = {};
		allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocInfo.allocationSize = memReqs.size;
		allocInfo.memoryTypeIndex = vkutils::FindMemoryType(physicalDevice, memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

		if (vkAllocateMemory(device, &allocInfo, nullptr, &imageMemory) != VK_SUCCESS)
		{
			std::cout << "Error -> Failed to allocate image memory!\n";
		}

		vkBindImageMemory(device, image, imageMemory, 0);
	}

	void VKTextureCube::LoadFaceIndividual(VkPhysicalDevice physicalDevice, VkDevice device)
	{
	}

	void VKTextureCube::CreateImageView(VkDevice device)
	{
		// Textures are not directly accessed by the shaders and
		// are abstracted by image views containing additional
		// information and sub resource ranges
		VkImageViewCreateInfo viewInfo = {};
		viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		viewInfo.image = image;
		viewInfo.viewType = VK_IMAGE_VIEW_TYPE_CUBE;
		viewInfo.format = format;
		viewInfo.components = { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A };
		// The subresource range describes the set of mip levels (and array layers) that can be accessed through this image view
		// It's possible to create multiple image views for a single image referring to different (and/or overlapping) ranges of the image
		viewInfo.subresourceRange.aspectMask = aspectFlags;
		viewInfo.subresourceRange.baseMipLevel = 0;
		viewInfo.subresourceRange.levelCount = static_cast<uint32_t>(mipLevels);
		viewInfo.subresourceRange.baseArrayLayer = 0;
		viewInfo.subresourceRange.layerCount = 6;			// 6 array layers (faces)

		if (vkCreateImageView(device, &viewInfo, nullptr, &imageView) != VK_SUCCESS)
		{
			std::cout << "Error -> Failed to create image view!\n";
		}
	}

	void VKTextureCube::CreateSampler(VkDevice device)
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
		samplerInfo.maxAnisotropy = 1.0f;							// More than 1 requires enabling GPU feature
		samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
		samplerInfo.unnormalizedCoordinates = VK_FALSE;				// If true it would be 0-texWidth/height instead of 0-1
		samplerInfo.compareEnable = VK_FALSE;						// If true texels will first be compared to a value, and the result of that comparison us used in filtering operations. Mainly used for PCF
		samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
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
