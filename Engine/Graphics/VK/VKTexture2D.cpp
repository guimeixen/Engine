#include "VKTexture2D.h"

#include "VKBase.h"
#include "Program/Log.h"
#include "VKBuffer.h"

#include "include/gli/gli.hpp"
#include "include/stb_image.h"
#include "include/half.hpp"

#include <iostream>

namespace Engine
{
	VKTexture2D::VKTexture2D()
	{
		width = 0;
		height = 0;
		sampler = VK_NULL_HANDLE;
		this->type = TextureType::TEXTURE2D;
		isAttachment = false;
		mipmapsGenerated = false;
		data = nullptr;
		stagingBuffer = nullptr;
	}

	VKTexture2D::VKTexture2D(VKBase *base, unsigned int width, unsigned int height, const TextureParams &params, const void *data)
	{
		AddReference();
		this->data = nullptr;
		this->allocator = base->GetAllocator();
		this->width = width;
		this->height = height;
		this->params = params;
		isAttachment = false;
		mipmapsGenerated = false;
		type = TextureType::TEXTURE2D;
		aspectFlags = VK_IMAGE_ASPECT_COLOR_BIT;
		sampler = VK_NULL_HANDLE;
		mipLevels = 1;
		addressMode = vkutils::GetAddressMode(params.wrap);
		filter = vkutils::GetFilter(params.filter);
		format = vkutils::GetFormat(params.internalFormat);
		stagingBuffer = nullptr;

		device = base->GetDevice();

		if (params.usedAsStorageInCompute || params.usedAsStorageInGraphics)
		{
			VkFormatProperties formatProps = {};
			vkGetPhysicalDeviceFormatProperties(base->GetPhysicalDevice(), format, &formatProps);
			if ((formatProps.optimalTilingFeatures & VK_FORMAT_FEATURE_STORAGE_IMAGE_BIT) == false)
			{
				std::cout << "Missing support for storage image bit\n";
				return;
			}
			if (data)
				usageFlags = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;		// TODO: How to know if the image is also going to be used for sampling too?
			else
				usageFlags = VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
		}
		else if (params.usedInCopy)
		{
			usageFlags = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
		}
		else
		{
			usageFlags = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;			// The image is going to be used as a dst for a buffer copy and we will also access it from the shader
		}

		unsigned int numChannels = Texture::GetNumChannels(params.internalFormat);
		if (params.type == TextureDataType::FLOAT)
		{
			size = width * height * numChannels * sizeof(float);
		}
		else if (params.type == TextureDataType::UNSIGNED_BYTE)
		{
			size = width * height * numChannels * sizeof(unsigned char);
		}		

		if (data)
		{
			// Because Vulkan doesn't support formats with 24 bits we convert to 32 bits
			// This assumes we're using RGB16F. Change texture data type to HALF_FLOAT?
			if (numChannels == 3 && params.type == TextureDataType::FLOAT)
			{
				format = vkutils::GetFormat(Texture::Get4ChannelEquivalent(params.internalFormat));

				half_float::half *newData = new half_float::half[width * height * 4];
				const float *oldData = static_cast<const float*>(data);
				unsigned int counter = 0;
				unsigned int counterOld = 0;

				size = width * height * 4 * sizeof(half_float::half);

				// Better way to copy?
				for (unsigned int i = 0; i < width * height; i++)
				{
					newData[counter++] = half_float::half_cast<half_float::half>(oldData[counterOld++]);
					newData[counter++] = half_float::half_cast<half_float::half>(oldData[counterOld++]);
					newData[counter++] = half_float::half_cast<half_float::half>(oldData[counterOld++]);
					newData[counter++] = half_float::half_cast<half_float::half>(0.0f);
				}

				stagingBuffer = new VKBuffer(base, nullptr, size, BufferType::StagingBuffer, BufferUsage::DYNAMIC);
				stagingBuffer->Map();
				stagingBuffer->Update(newData, (unsigned int)size, 0);
				stagingBuffer->Unmap();

				delete[] newData;
			}
			else
			{
				stagingBuffer = new VKBuffer(base, nullptr, size, BufferType::StagingBuffer, BufferUsage::DYNAMIC);
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
			region.imageExtent.depth = 1;
			region.bufferOffset = 0;

			bufferCopyRegions.push_back(region);
		}

		CreateImage(base->GetPhysicalDevice());
	}

	VKTexture2D::VKTexture2D(VKBase *base, const std::string &path, const TextureParams &params, bool storeTextureData)
	{
		AddReference();
		data = nullptr;
		width = 0;
		height = 0;
		isAttachment = false;
		this->path = path;
		this->params = params;
		this->storeTextureData = storeTextureData;
		type = TextureType::TEXTURE2D;
		aspectFlags = VK_IMAGE_ASPECT_COLOR_BIT;
		sampler = VK_NULL_HANDLE;
		format = vkutils::GetFormat(params.internalFormat);
		stagingBuffer = nullptr;

		device = base->GetDevice();
		allocator = base->GetAllocator();

		if (params.usedAsStorageInCompute || params.usedAsStorageInGraphics)
		{
			VkFormatProperties formatProps = {};
			vkGetPhysicalDeviceFormatProperties(base->GetPhysicalDevice(), format, &formatProps);
			if ((formatProps.optimalTilingFeatures & VK_FORMAT_FEATURE_STORAGE_IMAGE_BIT) == false)
			{
				std::cout << "Missing support for storage image bit\n";
				return;
			}

			usageFlags = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;		// TODO: How to know if the image is also going to be used for sampling too?
		}
		else
		{
			usageFlags = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;			// The image is going to be used as a dst for a buffer copy and we will also access it from the shader
		}
	}

	VKTexture2D::~VKTexture2D()
	{
		if (device)
			Dispose();

		DisposeStagingBuffer();
	}

	void VKTexture2D::Bind(unsigned int slot) const
	{
	}

	void VKTexture2D::BindAsImage(unsigned int slot, unsigned int mipLevel, bool layered, ImageAccess access, TextureInternalFormat format) const
	{
	}

	void VKTexture2D::Unbind(unsigned int slot) const
	{
	}

	void VKTexture2D::Clear()
	{
	}

	bool VKTexture2D::Load(VKBase* base)
	{	
		Log::Print(LogLevel::LEVEL_INFO, "Loading texture %s\n", path.c_str());

		if ((std::strstr(path.c_str(), ".png") > 0) || (std::strstr(path.c_str(), ".jpg") > 0))
			return LoadPNGJPG(base);

		if (storeTextureData)
		{
			Log::Print(LogLevel::LEVEL_WARNING, "Can't store data of compressed texture: %s\n", path.c_str());
			storeTextureData = false;
		}

		gli::texture t = gli::load(path);

		if (t.empty())
		{
			Log::Print(LogLevel::LEVEL_ERROR, "Failed to load texture: %s\n", path.c_str());
			path = "Data/Textures/white.dds";
			t = gli::load(path);

			if (t.empty())
				return false;
		}

		gli::texture2d tex2D(t);

		width = tex2D[0].extent().x;
		height = tex2D[0].extent().y;
		mipLevels = tex2D.levels();
		size = tex2D.size();
		addressMode = vkutils::GetAddressMode(params.wrap);
		filter = vkutils::GetFilter(params.filter);
		mipmapsGenerated = true;

		if (std::strstr(path.c_str(), ".dds") > 0)
		{
			if (tex2D.format() == gli::FORMAT_RGBA_DXT5_UNORM_BLOCK16)
				format = VK_FORMAT_BC3_UNORM_BLOCK;
			else
				format = VK_FORMAT_BC1_RGBA_UNORM_BLOCK;
		}
		else if (std::strstr(path.c_str(), ".ktx") > 0)
		{
			if (tex2D.format() == gli::FORMAT_RGB_DXT1_UNORM_BLOCK8)
			{
				format = VK_FORMAT_BC1_RGB_UNORM_BLOCK;
			}
			else if (tex2D.format() == gli::FORMAT_RGBA_DXT1_UNORM_BLOCK8)
			{
				format = VK_FORMAT_BC1_RGBA_UNORM_BLOCK;
			}
			else if (tex2D.format() == gli::FORMAT_RGBA_DXT5_UNORM_BLOCK16)
			{
				format = VK_FORMAT_BC3_UNORM_BLOCK;
			}		
		}
		else
		{
			Log::Print(LogLevel::LEVEL_ERROR, "Error -> Unsupported image extension!\n");
			return false;
		}

		stagingBuffer = new VKBuffer(base, nullptr, size, BufferType::StagingBuffer, BufferUsage::DYNAMIC);
		stagingBuffer->Map();
		stagingBuffer->Update(tex2D.data(), tex2D.size(), 0);
		stagingBuffer->Unmap();

		if (!CreateImage(base->GetPhysicalDevice()))
			return false;

		uint32_t offset = 0;

		// Setup buffer copy regions
		for (size_t i = 0; i < mipLevels; i++)
		{
			VkBufferImageCopy region = {};
			region.imageSubresource.aspectMask = aspectFlags;
			region.imageSubresource.mipLevel = i;
			region.imageSubresource.baseArrayLayer = 0;
			region.imageSubresource.layerCount = 1;
			region.imageExtent.width = static_cast<uint32_t>(tex2D[i].extent().x);
			region.imageExtent.height = static_cast<uint32_t>(tex2D[i].extent().y);
			region.imageExtent.depth = 1;
			region.bufferOffset = offset;

			bufferCopyRegions.push_back(region);

			offset += static_cast<uint32_t>(tex2D[i].size());
		}

		return true;
	}

	bool VKTexture2D::CreateColorAttachment(VKAllocator *allocator, VkPhysicalDevice physicalDevice, VkDevice device, uint32_t width, uint32_t height, const TextureParams &params)
	{
		this->width = width;
		this->height = height;
		this->params = params;
		this->device = device;
		this->allocator = allocator;
		isAttachment = true;
		mipLevels = 1;
		aspectFlags = VK_IMAGE_ASPECT_COLOR_BIT;

		if (params.usedInCopy)
		{
			usageFlags = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
		}
		else
		{
			usageFlags = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
		}

		filter = vkutils::GetFilter(params.filter);
		addressMode = vkutils::GetAddressMode(params.wrap);
		format = vkutils::GetFormat(params.internalFormat);

		if (!CreateImage(physicalDevice))
			return false;

		CreateImageView();
		CreateSampler();

		return true;
	}

	bool VKTexture2D::CreateDepthStencilAttachment(VKAllocator *allocator, VkPhysicalDevice physicalDevice, VkDevice device, uint32_t width, uint32_t height, const TextureParams &params, bool useInShader, bool useStencil)
	{
		this->width = width;
		this->height = height;
		this->device = device;
		this->allocator = allocator;
		this->params = params;
		isAttachment = true;
		mipLevels = 1;
		filter = vkutils::GetFilter(params.filter);
		addressMode = vkutils::GetAddressMode(params.wrap);

		VkFormatFeatureFlagBits features;

		if (useInShader)
		{
			usageFlags = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
			features = (VkFormatFeatureFlagBits)(VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT);
		}
		else
		{
			usageFlags = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
			features = VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT;
		}

		format = vkutils::GetFormat(params.internalFormat);
		format = vkutils::FindSupportedFormat(physicalDevice, { format }, VK_IMAGE_TILING_OPTIMAL, features);
		
		if (useStencil || vkutils::HasStencilComponent(format))
			aspectFlags = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
		else
			aspectFlags = VK_IMAGE_ASPECT_DEPTH_BIT;

		if (!CreateImage(physicalDevice))
			return false;

		CreateImageView();

		if (useInShader)
			CreateSampler();

		return true;
	}

	void VKTexture2D::Dispose()
	{
		if (data)
			delete[] data;

		data = nullptr;

		if (sampler != VK_NULL_HANDLE)
			vkDestroySampler(device, sampler, nullptr);
		if (imageView != VK_NULL_HANDLE)
			vkDestroyImageView(device, imageView, nullptr);
		if (image != VK_NULL_HANDLE)
			vkDestroyImage(device, image, nullptr);

		allocator->Free(alloc);
	}

	void VKTexture2D::DisposeStagingBuffer()
	{
		if (stagingBuffer)
		{
			delete stagingBuffer;
			stagingBuffer = nullptr;
		}
	}

	bool VKTexture2D::CreateImage(VkPhysicalDevice physicalDevice)
	{
		VkImageCreateInfo imageInfo = {};
		imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		imageInfo.arrayLayers = 1;
		imageInfo.extent.width = width;
		imageInfo.extent.height = height;
		imageInfo.extent.depth = 1;
		imageInfo.imageType = VK_IMAGE_TYPE_2D;
		imageInfo.mipLevels = mipLevels;
		imageInfo.usage = usageFlags;
		imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
		imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		imageInfo.format = format;
		imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
		imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

		if (params.imageViewsWithDifferentFormats)
			imageInfo.flags = VK_IMAGE_CREATE_MUTABLE_FORMAT_BIT;
		else
			imageInfo.flags = 0;

		if (vkCreateImage(device, &imageInfo, nullptr, &image) != VK_SUCCESS)
		{
			Log::Print(LogLevel::LEVEL_ERROR, "Error -> Failed to create image!\n");
			return false;
		}

		VkMemoryRequirements memReqs;
		vkGetImageMemoryRequirements(device, image, &memReqs);

		allocator->Allocate(alloc, memReqs.size, vkutils::FindMemoryType(physicalDevice, memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT), false);

		vkBindImageMemory(device, image, alloc.memory, alloc.offset);

		return true;
	}

	bool VKTexture2D::LoadPNGJPG(VKBase* base)
	{
		unsigned char* image = nullptr;
		int width, height, nChannels;

		if (params.format == TextureFormat::RGB)
		{
			image = stbi_load(path.c_str(), &width, &height, nullptr, STBI_rgb);
			nChannels = 3;
		}
		else if (params.format == TextureFormat::RGBA)
		{
			image = stbi_load(path.c_str(), &width, &height, nullptr, STBI_rgb_alpha);
			nChannels = 4;
		}
		else if (params.format == TextureFormat::RED)
		{
			image = stbi_load(path.c_str(), &width, &height, nullptr, STBI_grey);
			nChannels = 1;
		}

		if (!image)
		{
			Log::Print(LogLevel::LEVEL_ERROR, "Failed to load texture: %s\n",path.c_str());
			path = "Data/Textures/white.dds";
			return Load(base);
		}
		
		mipmapsGenerated = false;
		this->width = static_cast<uint32_t>(width);
		this->height = static_cast<uint32_t>(height);
		addressMode = vkutils::GetAddressMode(params.wrap);
		filter = vkutils::GetFilter(params.filter);
		format = vkutils::GetFormat(params.internalFormat);
		size = width * height * nChannels;

		VkFormatProperties formatProps;
		vkGetPhysicalDeviceFormatProperties(base->GetPhysicalDevice(), format, &formatProps);	// We need to check if this format supports blits

		if (params.useMipmapping && (formatProps.optimalTilingFeatures & VK_FORMAT_FEATURE_BLIT_SRC_BIT) == VK_FORMAT_FEATURE_BLIT_SRC_BIT && (formatProps.optimalTilingFeatures & VK_FORMAT_FEATURE_BLIT_DST_BIT) == VK_FORMAT_FEATURE_BLIT_DST_BIT)
		{
			mipLevels = static_cast<size_t>(std::floor(std::log2(std::max(width, height)))) + 1;
			usageFlags |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;		// The transfer_dst and sampled flags are already set
		}
		else
		{
			mipLevels = 1;
		}



		if (params.internalFormat == TextureInternalFormat::R16F)
		{
			half_float::half *d = new half_float::half[width * height * 1];
			size_t s = width * height;
			for (size_t i = 0; i < s; i++)
			{
				d[i] = half_float::half_cast<half_float::half>(image[i]);
				//d[i] = 0.0f;
			}

			size *= sizeof(half_float::half);

			stagingBuffer = new VKBuffer(base, nullptr, size, BufferType::StagingBuffer, BufferUsage::DYNAMIC);
			stagingBuffer->Map();
			stagingBuffer->Update(d, (unsigned int)size, 0);
			stagingBuffer->Unmap();

			if (storeTextureData)
			{
				data = new unsigned char[width * height * nChannels];
				memcpy(data, image, width * height * nChannels * sizeof(unsigned char));
			}

			delete[] d;
		}
		else
		{
			size *= sizeof(unsigned char);

			stagingBuffer = new VKBuffer(base, nullptr, size, BufferType::StagingBuffer, BufferUsage::DYNAMIC);
			stagingBuffer->Map();
			stagingBuffer->Update(image, (unsigned int)size, 0);
			stagingBuffer->Unmap();

			if (storeTextureData)
			{
				data = new unsigned char[width * height * nChannels];
				memcpy(data, image, width * height * nChannels * sizeof(unsigned char));
			}
		}	

		if (!CreateImage(base->GetPhysicalDevice()))
			return false;

		// Setup buffer copy regions
		// We only use one here because we only have the base texture and not the whole mip chain
		VkBufferImageCopy region = {};
		region.imageSubresource.aspectMask = aspectFlags;
		region.imageSubresource.mipLevel = 0;
		region.imageSubresource.baseArrayLayer = 0;
		region.imageSubresource.layerCount = 1;
		region.imageExtent.width = static_cast<uint32_t>(width);
		region.imageExtent.height = static_cast<uint32_t>(height);
		region.imageExtent.depth = 1;
		region.bufferOffset = 0;

		bufferCopyRegions.push_back(region);

		stbi_image_free(image);

		return true;
	}

	void VKTexture2D::CreateImageView()
	{
		// Textures are not directly accessed by the shaders and
		// are abstracted by image views containing additional
		// information and sub resource ranges
		VkImageViewCreateInfo viewInfo = {};
		viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		viewInfo.image = image;
		viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		viewInfo.format = format;
		viewInfo.components = { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A };
		// The subresource range describes the set of mip levels (and array layers) that can be accessed through this image view
		// It's possible to create multiple image views for a single image referring to different (and/or overlapping) ranges of the image
		if (IsDepthTexture())
			viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
		else
			viewInfo.subresourceRange.aspectMask = aspectFlags;
		viewInfo.subresourceRange.baseMipLevel = 0;
		viewInfo.subresourceRange.levelCount = static_cast<uint32_t>(mipLevels);
		viewInfo.subresourceRange.baseArrayLayer = 0;
		viewInfo.subresourceRange.layerCount = 1;

		if (vkCreateImageView(device, &viewInfo, nullptr, &imageView) != VK_SUCCESS)
		{
			std::cout << "Error -> Failed to create image view!\n";
		}
	}

	void VKTexture2D::CreateSampler()
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

		if (params.enableCompare)
		{
			samplerInfo.compareEnable = VK_TRUE;					// If true texels will first be compared to a value, and the result of that comparison us used in filtering operations. Mainly used for PCF
			samplerInfo.compareOp = VK_COMPARE_OP_LESS;
			samplerInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
		}
		else
		{
			samplerInfo.compareEnable = VK_FALSE;
			samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
		}

		
		samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
		samplerInfo.mipLodBias = 0.0f;
		samplerInfo.minLod = 0.0f;
		samplerInfo.maxLod = (float)mipLevels;

		if (vkCreateSampler(device, &samplerInfo, nullptr, &sampler) != VK_SUCCESS)
		{
			std::cout << "Error -> Failed to create sampler!\n";
		}
	}

	VkBuffer VKTexture2D::GetStagingBuffer() const
	{
		if (stagingBuffer)
			return stagingBuffer->GetBuffer();

		return VK_NULL_HANDLE;
	}
}
