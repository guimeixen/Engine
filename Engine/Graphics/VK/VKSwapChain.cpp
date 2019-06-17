#include "VKSwapChain.h"

#include "VKUtils.h"
#include "Program\Log.h"
#include "VKTexture2D.h"

namespace Engine
{
	VKSwapChain::VKSwapChain()
	{
	}

	VKSwapChain::~VKSwapChain()
	{
	}

	bool VKSwapChain::Init(VKAllocator *allocator, VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, VkDevice device, uint32_t width, uint32_t height)
	{
		this->width = width;
		this->height = height;

		if (!CreateSwapChain(physicalDevice, surface, device))
			return false;

		CreateSwapChainImages(device);

		if (!CreateSwapChainImageViews(device))
			return false;

		depthTexture = new VKTexture2D();
		depthTexture->CreateDepthStencilAttachment(allocator, physicalDevice, device, extent.width, extent.height, { TextureWrap::CLAMP_TO_EDGE, TextureFilter::LINEAR, TextureFormat::DEPTH_COMPONENT, TextureInternalFormat::DEPTH_COMPONENT24, TextureDataType::FLOAT, false, false }, false, true);
		depthTexture->AddReference();

		return true;
	}

	bool VKSwapChain::Recreate(VKAllocator *allocator, VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, VkDevice device, uint32_t width, uint32_t height)
	{
		this->width = width;
		this->height = height;

		Dispose(device);

		if (!CreateSwapChain(physicalDevice, surface, device))
			return false;

		CreateSwapChainImages(device);

		if (!CreateSwapChainImageViews(device))
			return false;

		depthTexture = new VKTexture2D();
		depthTexture->CreateDepthStencilAttachment(allocator, physicalDevice, device, extent.width, extent.height, { TextureWrap::CLAMP_TO_EDGE, TextureFilter::LINEAR, TextureFormat::DEPTH_COMPONENT, TextureInternalFormat::DEPTH_COMPONENT24, TextureDataType::FLOAT, false, false }, false, true);
		depthTexture->AddReference();

		return true;
	}

	void VKSwapChain::Dispose(VkDevice device)
	{
		for (size_t i = 0; i < framebuffers.size(); i++)
		{
			vkDestroyFramebuffer(device, framebuffers[i], nullptr);
		}

		depthTexture->RemoveReference();

		for (size_t i = 0; i < imageViews.size(); i++)
		{
			vkDestroyImageView(device, imageViews[i], nullptr);
		}
		// The images were created by the implementation for the swap chain and they will 
		// be automatically cleaned up once the swap chain has been destroyed

		vkDestroySwapchainKHR(device, swapChain, nullptr);
	}

	bool VKSwapChain::CreateSwapChain(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, VkDevice device)
	{
		SwapChainSupportDetails swapChainSupport = vkutils::QuerySwapChainSupport(physicalDevice, surface);

		surfaceFormat = vkutils::ChooseSwapSurfaceFormat(swapChainSupport.formats);
		VkPresentModeKHR presentMode = vkutils::ChooseSwapPresentMode(swapChainSupport.presentModes);
		extent = vkutils::ChooseSwapExtent(swapChainSupport.capabilities, width, height);

		uint32_t imageCount = swapChainSupport.capabilities.minImageCount;

		// maxImageCount == 0 means that there is no limit besides memory requirements which is why we need to check
		if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount)
		{
			imageCount = swapChainSupport.capabilities.maxImageCount;
		}

		VkSwapchainCreateInfoKHR swapChainInfo = {};
		swapChainInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
		swapChainInfo.surface = surface;
		swapChainInfo.minImageCount = imageCount;
		swapChainInfo.imageFormat = surfaceFormat.format;
		swapChainInfo.imageColorSpace = surfaceFormat.colorSpace;
		swapChainInfo.imageExtent = extent;
		swapChainInfo.imageArrayLayers = 1;
		swapChainInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
		swapChainInfo.preTransform = swapChainSupport.capabilities.currentTransform;	// We could flip the images or do other things but we don't want to so use the currentTransform
		swapChainInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;			// Used for blending with other windows. We almost always want to ignore the alpha channel so use Opaque
		swapChainInfo.presentMode = presentMode;
		swapChainInfo.clipped = VK_TRUE;		// If true then we don't care about the color of pixels that are obscured, eg. because another window is in front of them. Best performance with it enabled
		swapChainInfo.oldSwapchain = VK_NULL_HANDLE;

		QueueFamilyIndices indices = vkutils::FindQueueFamilies(physicalDevice, surface);

		uint32_t queueFamilyIndices[] = { (uint32_t)indices.graphicsFamily, (uint32_t)indices.presentFamily };

		if (indices.graphicsFamily != indices.presentFamily)
		{
			swapChainInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
			swapChainInfo.queueFamilyIndexCount = 2;
			swapChainInfo.pQueueFamilyIndices = queueFamilyIndices;
		}
		else {
			swapChainInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;		// Best performance
			swapChainInfo.queueFamilyIndexCount = 0;
			swapChainInfo.pQueueFamilyIndices = nullptr;
		}

		if (vkCreateSwapchainKHR(device, &swapChainInfo, nullptr, &swapChain) != VK_SUCCESS)
		{
			Log::Print(LogLevel::LEVEL_ERROR, "Failed to create swapchain\n");
			return false;
		}

		return true;
	}

	void VKSwapChain::CreateSwapChainImages(VkDevice device)
	{
		uint32_t swapChainImageCount = 0;
		vkGetSwapchainImagesKHR(device, swapChain, &swapChainImageCount, nullptr);
		images.resize(swapChainImageCount);
		vkGetSwapchainImagesKHR(device, swapChain, &swapChainImageCount, images.data());
	}

	bool VKSwapChain::CreateSwapChainImageViews(VkDevice device)
	{
		// Image views
		imageViews.resize(images.size());

		for (size_t i = 0; i < images.size(); i++)
		{
			VkImageViewCreateInfo viewInfo = {};
			viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			viewInfo.image = images[i];
			viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
			viewInfo.format = surfaceFormat.format;
			viewInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
			viewInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
			viewInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
			viewInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
			viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			viewInfo.subresourceRange.baseMipLevel = 0;
			viewInfo.subresourceRange.levelCount = 1;
			viewInfo.subresourceRange.baseArrayLayer = 0;
			viewInfo.subresourceRange.layerCount = 1;

			if (vkCreateImageView(device, &viewInfo, nullptr, &imageViews[i]) != VK_SUCCESS)
			{
				Log::Print(LogLevel::LEVEL_ERROR, "Failed to create swapchain image view with index: %d\n", i);
				return false;
			}
		}

		return true;
	}

	void VKSwapChain::CreateFramebuffers(VkDevice device, VkRenderPass renderPass)
	{
		// Frambuffer
		framebuffers.resize(imageViews.size());

		for (size_t i = 0; i < imageViews.size(); i++)
		{
			VkImageView attachments[] = { imageViews[i], depthTexture->GetImageView() };

			VkFramebufferCreateInfo framebufferInfo = {};
			framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			framebufferInfo.renderPass = renderPass;
			framebufferInfo.attachmentCount = 2;
			framebufferInfo.pAttachments = attachments;
			framebufferInfo.width = extent.width;
			framebufferInfo.height = extent.height;
			framebufferInfo.layers = 1;

			if (vkCreateFramebuffer(device, &framebufferInfo, nullptr, &framebuffers[i]) != VK_SUCCESS)
			{
				Log::Print(LogLevel::LEVEL_ERROR, "Failed to create swapchain framebuffers\n");
			}
		}
	}
}
