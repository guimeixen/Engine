#pragma once

#include <vulkan\vulkan.h>
#include "include\GLFW\glfw3.h"

#include "VKUtils.h"
#include "VKAllocator.h"

#include <vector>

namespace Engine
{
	class VKTexture2D;
	class VKTexture3D;
	class VKTextureCube;

	class VKBase
	{
	public:
		VKBase();
		~VKBase();

		bool Init(GLFWwindow *window);
		void Dispose();

		void BeginSingleTimeCommands();
		void EndSingleTimeCommands();
		VkCommandBuffer AllocateCommandBuffer();
		VkCommandBuffer AllocateComputeCommandBuffer();
		std::vector<VkCommandBuffer> AllocateCommandBuffers(size_t count);
		void FreeCommandBuffer(VkCommandBuffer commandBuffer);
		void FreeCommandBuffers(const std::vector<VkCommandBuffer> &commandBuffers);
		void FreeComputeCommandBuffers(VkCommandBuffer commandBuffer);

		void CopyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
		void BlitImage(VKTexture2D *tex2D, const VkImageBlit &blit);

		void TransitionImageLayout(VkCommandBuffer transferCb, const VKTexture2D *texture, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t mipLevels, uint32_t baseMipLevel = 0);
		void TransitionImageLayout(VkCommandBuffer transferCb, const VKTexture3D *texture, VkImageLayout oldLayout, VkImageLayout newLayout);
		void TransitionImageLayout(VkCommandBuffer transferCb, const VKTextureCube *texture, VkImageLayout oldLayout, VkImageLayout newLayout);

		void ShowAvailableExtensions();

		const VkInstance &GetInstance() const { return instance; }
		VkSurfaceKHR GetSurface() const { return surface; }
		const VkPhysicalDevice &GetPhysicalDevice() const { return physicalDevice; }
		const VkDevice &GetDevice() const { return device; }

		VkCommandBuffer GetSingleTimeCommandbuffer() const { return singleTimeCmdBuffer; }

		VKAllocator *GetAllocator() const { return vkAllocator; }

		const VkQueue &GetGraphicsQueue() const { return graphicsQueue; }
		const VkQueue &GetPresentQueue() const { return presentQueue; }
		const VkQueue &GetTransferQueue() const { return transferQueue; }
		const VkQueue &GetComputeQueue() const { return computeQueue; }
		uint32_t GetGraphicsQueueFamily() const { return indices.graphicsFamily; }

		VkPhysicalDeviceFeatures GetDeviceFeatures() const { return deviceFeatures; }
		VkPhysicalDeviceLimits GetDeviceLimits() const { return deviceProperties.limits; }
		VkPhysicalDeviceMemoryProperties GetMemoryProperties() const { return gpuMemoryProperties; }

	private:
		void RealTransitionImageLayout(VkImageMemoryBarrier &imageMemBarrier, VkImageLayout oldLayout, VkImageLayout newLayout);

	private:
		bool CreateInstance();
		bool CreateDebugReportCallback();
		bool CreateSurface(GLFWwindow *window);
		bool CreatePhysicalDevice();
		bool CreateLogicalDevice();
		bool CreateCommandPool();

	private:
		VkInstance instance;
		VkDebugReportCallbackEXT callback;
		VkSurfaceKHR surface;
		VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
		VkDevice device;

		VKAllocator *vkAllocator;

		QueueFamilyIndices indices;

		VkQueue graphicsQueue;
		VkQueue computeQueue;
		VkQueue presentQueue;
		VkQueue transferQueue;

		VkCommandPool cmdPool;
		VkCommandPool computeCmdPool;
		VkCommandBuffer singleTimeCmdBuffer;

		VkPhysicalDeviceFeatures deviceFeatures;
		VkPhysicalDeviceProperties deviceProperties;
		VkPhysicalDeviceMemoryProperties gpuMemoryProperties;

		const std::vector<const char*> validationLayers = { "VK_LAYER_KHRONOS_validation" };
		const std::vector<const char*> deviceExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

#ifdef NDEBUG
		const bool enableValidationLayers = false;
#else
		const bool enableValidationLayers = true;
#endif
	};
}
