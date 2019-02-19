#pragma once

#include "VKTexture2D.h"

#include <vector>

namespace Engine
{
	class VKSwapChain
	{
	public:
		VKSwapChain();
		~VKSwapChain();

		bool Init(VKAllocator *allocator, VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, VkDevice device, uint32_t width, uint32_t height);
		bool Recreate(VKAllocator *allocator, VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, VkDevice device, uint32_t width, uint32_t height);
		void Dispose(VkDevice device);

		void CreateFramebuffers(VkDevice device, VkRenderPass renderPass);

		VkSwapchainKHR GetSwapChain() const { return swapChain; }
		VkFormat GetSurfaceFormat() const { return surfaceFormat.format; }
		VkFormat GetDepthFormat() const { return depthTexture->GetFormat(); }
		VkExtent2D GetExtent() const { return extent; }
		size_t GetImageCount() const { return imageViews.size(); }
		VkFramebuffer GetFramebuffer(size_t index) const { return framebuffers[index]; }
		VKTexture2D *GetDepthTexture() const { return depthTexture; }

	private:
		bool CreateSwapChain(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, VkDevice device);
		void CreateSwapChainImages(VkDevice device);
		bool CreateSwapChainImageViews(VkDevice device);

	private:
		VkSwapchainKHR swapChain;

		VkSurfaceFormatKHR surfaceFormat;
		VkExtent2D extent;

		std::vector<VkImage> images;
		std::vector<VkImageView> imageViews;

		std::vector<VkFramebuffer> framebuffers;

		VKTexture2D *depthTexture;

		uint32_t width;
		uint32_t height;
	};
}
