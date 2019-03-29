#include "VKBase.h"

#include "VKDebug.h"
#include "Program\Log.h"
#include "VKTexture2D.h"
#include "VKTexture3D.h"
#include "VKTextureCube.h"

#include <set>
#include <iostream>

namespace Engine
{
	VKBase::VKBase()
	{
		vkAllocator = nullptr;
	}

	VKBase::~VKBase()
	{
	}

	bool VKBase::Init(GLFWwindow *window)
	{
		if (enableValidationLayers && !vkutils::ValidationLayersSupported(validationLayers))
		{
			Log::Print(LogLevel::LEVEL_ERROR, "Validation layers were requested, but are not available!");
			return false;
		}

		if (!CreateInstance())
			return false;
		if (enableValidationLayers && !CreateDebugReportCallback())
			return false;
		if (!CreateSurface(window))
			return false;
		if (!CreatePhysicalDevice())
			return false;
		if (!CreateLogicalDevice())
			return false;
		if (!CreateCommandPool())
			return false;

		vkAllocator = new VKAllocator(this);

		return true;
	}

	void VKBase::Dispose()
	{
		vkDestroyCommandPool(device, cmdPool, nullptr);
		vkDestroyCommandPool(device, computeCmdPool, nullptr);
		vkDestroyDevice(device, nullptr);
		vkDestroySurfaceKHR(instance, surface, nullptr);
		vkdebug::DestroyDebugReportCallbackEXT(instance, callback, nullptr);
		vkDestroyInstance(instance, nullptr);

		if (vkAllocator)
		{
			vkAllocator->PrintStats();
			delete vkAllocator;
		}
	}

	void VKBase::BeginSingleTimeCommands()
	{
		singleTimeCmdBuffer = AllocateCommandBuffer();

		VkCommandBufferBeginInfo beginInfo = {};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

		vkBeginCommandBuffer(singleTimeCmdBuffer, &beginInfo);
	}

	void VKBase::EndSingleTimeCommands()
	{
		vkEndCommandBuffer(singleTimeCmdBuffer);

		VkSubmitInfo submitInfo = {};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &singleTimeCmdBuffer;

		vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
		vkQueueWaitIdle(graphicsQueue);

		FreeCommandBuffer(singleTimeCmdBuffer);
	}

	VkCommandBuffer VKBase::AllocateCommandBuffer()
	{
		VkCommandBufferAllocateInfo allocInfo = {};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.commandPool = cmdPool;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandBufferCount = 1;

		VkCommandBuffer commandBuffer;
		if (vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer) != VK_SUCCESS)
		{
			Log::Print(LogLevel::LEVEL_ERROR, "Failed to allocate command buffer!");
		}

		return commandBuffer;
	}

	VkCommandBuffer VKBase::AllocateComputeCommandBuffer()
	{
		VkCommandBufferAllocateInfo allocInfo = {};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.commandPool = computeCmdPool;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandBufferCount = 1;

		VkCommandBuffer cmdBuffer;
		if (vkAllocateCommandBuffers(device, &allocInfo, &cmdBuffer) != VK_SUCCESS)
		{
			Log::Print(LogLevel::LEVEL_ERROR, "Failed to allocate compute command buffer!");
		}

		return cmdBuffer;
	}

	std::vector<VkCommandBuffer> VKBase::AllocateCommandBuffers(size_t count)
	{
		std::vector<VkCommandBuffer> commandBuffers;
		commandBuffers.resize(count);			// Commands are recorded for each swap chain image

		VkCommandBufferAllocateInfo allocInfo = {};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.commandPool = cmdPool;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandBufferCount = (uint32_t)commandBuffers.size();

		if (vkAllocateCommandBuffers(device, &allocInfo, commandBuffers.data()) != VK_SUCCESS)
		{
			Log::Print(LogLevel::LEVEL_ERROR, "Failed to allocate command buffers!");
		}

		return commandBuffers;
	}

	void VKBase::FreeCommandBuffer(VkCommandBuffer commandBuffer)
	{
		vkFreeCommandBuffers(device, cmdPool, 1, &commandBuffer);
	}

	void VKBase::FreeCommandBuffers(const std::vector<VkCommandBuffer>& commandBuffers)
	{
		vkFreeCommandBuffers(device, cmdPool, static_cast<uint32_t>(commandBuffers.size()), commandBuffers.data());
	}

	void VKBase::FreeComputeCommandBuffers(VkCommandBuffer commandBuffer)
	{
		vkFreeCommandBuffers(device, computeCmdPool, 1, &commandBuffer);
	}

	void VKBase::RealTransitionImageLayout(VkImageMemoryBarrier &imageMemBarrier, VkImageLayout oldLayout, VkImageLayout newLayout)
	{
		VkPipelineStageFlags sourceStage;
		VkPipelineStageFlags destinationStage;

		if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
		{
			imageMemBarrier.srcAccessMask = 0;
			imageMemBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

			sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
			destinationStage = VK_PIPELINE_STAGE_HOST_BIT;
		}
		else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL)
		{
			imageMemBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			imageMemBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

			sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
			destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		}
		else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
		{
			imageMemBarrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
			imageMemBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

			sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
			destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		}
		else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
		{
			imageMemBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			imageMemBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

			sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
			destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		}
		else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
		{
			imageMemBarrier.srcAccessMask = 0;
			imageMemBarrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

			sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
			destinationStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;		// Pick the earliest pipeline stage that matches the specified operations
		}
		else
		{
			Log::Print(LogLevel::LEVEL_ERROR, "Unsupported layout transition!");
		}

		vkCmdPipelineBarrier(singleTimeCmdBuffer, sourceStage, destinationStage, 0, 0, nullptr, 0, nullptr, 1, &imageMemBarrier);
	}

	void VKBase::CopyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size)
	{
		VkBufferCopy copyRegion = {};
		copyRegion.srcOffset = 0;
		copyRegion.dstOffset = 0;
		copyRegion.size = size;
		vkCmdCopyBuffer(singleTimeCmdBuffer, srcBuffer, dstBuffer, 1, &copyRegion);
	}

	void VKBase::BlitImage(VKTexture2D *tex2D, const VkImageBlit &blit)
	{
		vkCmdBlitImage(singleTimeCmdBuffer, tex2D->GetImage(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, tex2D->GetImage(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &blit, VK_FILTER_LINEAR);
	}

	void VKBase::TransitionImageLayout(VkCommandBuffer transferCb, const VKTexture2D *texture, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t mipLevels, uint32_t baseMipLevel)
	{
		VkImageMemoryBarrier barrier = {};
		barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		barrier.oldLayout = oldLayout;									// Could be VK_IMAGE_LAYOUT_UNDEFINED if we don't care about the existing contents of the image
		barrier.newLayout = newLayout;
		barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;			// If transfering queue family ownership this should be their indices
		barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.image = texture->GetImage();
		barrier.subresourceRange.aspectMask = texture->GetAspectFlags();
		barrier.subresourceRange.baseArrayLayer = 0;
		barrier.subresourceRange.baseMipLevel = baseMipLevel;
		barrier.subresourceRange.levelCount = mipLevels;			// We don't use texture->GetMipLevels() because for textures that wants mipmaps to be generated this has to be 1
		barrier.subresourceRange.layerCount = 1;

		VkPipelineStageFlags sourceStage;
		VkPipelineStageFlags destinationStage;

		if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
		{
			barrier.srcAccessMask = 0;
			barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

			sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
			destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		}
		else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL)
		{
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

			sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
			destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		}
		else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
		{
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
			barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

			sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
			destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		}
		else if (oldLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL)
		{
			barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
			barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

			sourceStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
			destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		}
		else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
		{
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

			sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
			destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		}
		else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
		{
			barrier.srcAccessMask = 0;
			barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

			sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
			destinationStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;		// Pick the earliest pipeline stage that matches the specified operations
		}
		else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_GENERAL)
		{
			barrier.srcAccessMask = 0;
			barrier.dstAccessMask = 0;

			sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
			destinationStage = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
		}
		else
		{
			Log::Print(LogLevel::LEVEL_ERROR, "Unsupported layout transition!");
		}

		vkCmdPipelineBarrier(transferCb, sourceStage, destinationStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);
	}

	void VKBase::TransitionImageLayout(VkCommandBuffer transferCb, const VKTexture3D * texture, VkImageLayout oldLayout, VkImageLayout newLayout)
	{
		VkImageMemoryBarrier barrier = {};
		barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		barrier.oldLayout = oldLayout;									// Could be VK_IMAGE_LAYOUT_UNDEFINED if we don't care about the existing contents of the image
		barrier.newLayout = newLayout;
		barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;			// If transfering queue family ownership this should be their indices
		barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.image = texture->GetImage();
		barrier.subresourceRange.aspectMask = texture->GetAspectFlags();
		barrier.subresourceRange.baseArrayLayer = 0;
		barrier.subresourceRange.baseMipLevel = 0;
		barrier.subresourceRange.levelCount = texture->GetMipLevels();
		barrier.subresourceRange.layerCount = 1;

		VkPipelineStageFlags sourceStage;
		VkPipelineStageFlags destinationStage;

		if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
		{
			barrier.srcAccessMask = 0;
			barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

			sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
			destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		}
		else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL)
		{
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

			sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
			destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		}
		else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
		{
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
			barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

			sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
			destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		}
		else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
		{
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

			sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
			destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		}
		else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
		{
			barrier.srcAccessMask = 0;
			barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

			sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
			destinationStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;		// Pick the earliest pipeline stage that matches the specified operations
		}
		else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_GENERAL)
		{
			barrier.srcAccessMask = 0;
			barrier.dstAccessMask = 0;

			sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
			destinationStage = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
		}
		else
		{
			Log::Print(LogLevel::LEVEL_ERROR, "Unsupported layout transition!");
		}

		vkCmdPipelineBarrier(transferCb, sourceStage, destinationStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);
	}

	void VKBase::TransitionImageLayout(VkCommandBuffer transferCb, const VKTextureCube * texture, VkImageLayout oldLayout, VkImageLayout newLayout)
	{
		VkImageMemoryBarrier barrier = {};
		barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		barrier.oldLayout = oldLayout;									// Could be VK_IMAGE_LAYOUT_UNDEFINED if we don't care about the existing contents of the image
		barrier.newLayout = newLayout;
		barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;			// If transfering queue family ownership this should be their indices
		barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.image = texture->GetImage();
		barrier.subresourceRange.aspectMask = texture->GetAspectFlags();
		barrier.subresourceRange.baseArrayLayer = 0;
		barrier.subresourceRange.baseMipLevel = 0;
		barrier.subresourceRange.levelCount = texture->GetMipLevels();
		barrier.subresourceRange.layerCount = 6;			// Cube map

		VkPipelineStageFlags sourceStage;
		VkPipelineStageFlags destinationStage;

		if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
		{
			barrier.srcAccessMask = 0;
			barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

			sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
			destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		}
		else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL)
		{
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

			sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
			destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		}
		else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
		{
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
			barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

			sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
			destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		}
		else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
		{
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

			sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
			destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		}
		else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
		{
			barrier.srcAccessMask = 0;
			barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

			sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
			destinationStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;		// Pick the earliest pipeline stage that matches the specified operations
		}
		else
		{
			Log::Print(LogLevel::LEVEL_ERROR, "Unsupported layout transition!");
		}

		vkCmdPipelineBarrier(transferCb, sourceStage, destinationStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);
	}

	void VKBase::ShowAvailableExtensions()
	{
		uint32_t extensionCount = 0;
		vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);

		std::vector<VkExtensionProperties> extensions(extensionCount);
		vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data());

		Log::Print(LogLevel::LEVEL_INFO, "Available Extensions:");
		for (const auto &extension : extensions)
		{
			Log::Print(LogLevel::LEVEL_INFO, "\t%s", extension.extensionName);
		}
	}

	bool VKBase::CreateInstance()
	{
		VkApplicationInfo appInfo = {};
		appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		appInfo.pApplicationName = "Test";
		appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.pEngineName = "Engine";
		appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.apiVersion = VK_API_VERSION_1_0;

		auto requiredExtentions = vkutils::GetRequiredExtensions(enableValidationLayers);

		VkInstanceCreateInfo instanceInfo = {};
		instanceInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		instanceInfo.pApplicationInfo = &appInfo;
		instanceInfo.enabledExtensionCount = static_cast<uint32_t>(requiredExtentions.size());
		instanceInfo.ppEnabledExtensionNames = requiredExtentions.data();

		if (enableValidationLayers)
		{
			instanceInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
			instanceInfo.ppEnabledLayerNames = validationLayers.data();
		}
		else
		{
			instanceInfo.enabledLayerCount = 0;
		}

		if (vkCreateInstance(&instanceInfo, nullptr, &instance) != VK_SUCCESS)
		{
			Log::Print(LogLevel::LEVEL_ERROR, "Failed to create Vulkan Instance!");
			return false;
		}

		return true;
	}

	bool VKBase::CreateDebugReportCallback()
	{
		// SETUP DEBUG CALLBACK
		VkDebugReportCallbackCreateInfoEXT callbackInfo = {};
		callbackInfo.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
		callbackInfo.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT | VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT;
		callbackInfo.pfnCallback = vkdebug::DebugCallback;

		if (vkdebug::CreateDebugReportCallbackEXT(instance, &callbackInfo, nullptr, &callback) != VK_SUCCESS)
		{
			Log::Print(LogLevel::LEVEL_ERROR, "Failed to create debug report callback!");
			return false;
		}

		return true;
	}

	bool VKBase::CreateSurface(GLFWwindow *window)
	{
		// SURFACE CREATION
		if (glfwCreateWindowSurface(instance, window, nullptr, &surface) != VK_SUCCESS)
		{
			Log::Print(LogLevel::LEVEL_ERROR, "Failed to create window surface!");
			return false;
		}

		return true;
	}

	bool VKBase::CreatePhysicalDevice()
	{
		// No need to destroy the physical device because it is implicitly destroyed when destroying the instance
		uint32_t deviceCount = 0;
		vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);

		if (deviceCount == 0)
		{
			Log::Print(LogLevel::LEVEL_ERROR, "Failed to find GPUs with Vulkan support!");
			return false;
		}

		std::vector<VkPhysicalDevice> physicalDevices(deviceCount);
		vkEnumeratePhysicalDevices(instance, &deviceCount, physicalDevices.data());

		for (const auto &gpu : physicalDevices)
		{
			if (vkutils::IsPhysicalDeviceSuitable(gpu, surface, deviceExtensions))
			{
				physicalDevice = gpu;
				break;
			}
		}

		if (physicalDevice == VK_NULL_HANDLE)
		{
			Log::Print(LogLevel::LEVEL_ERROR, "Failed to find suitable GPU!");
			return false;
		}

		vkGetPhysicalDeviceFeatures(physicalDevice, &deviceFeatures);
		vkGetPhysicalDeviceProperties(physicalDevice, &deviceProperties);
		vkGetPhysicalDeviceMemoryProperties(physicalDevice, &gpuMemoryProperties);

		Log::Print(LogLevel::LEVEL_INFO, "Non coherent atom size: %d", deviceProperties.limits.nonCoherentAtomSize);
		Log::Print(LogLevel::LEVEL_INFO, "Max allocations: %d", deviceProperties.limits.maxMemoryAllocationCount);
		Log::Print(LogLevel::LEVEL_INFO, "Memory heaps: %d", gpuMemoryProperties.memoryHeapCount);

		for (uint32_t i = 0; i < gpuMemoryProperties.memoryHeapCount; i++)
		{			
			if (gpuMemoryProperties.memoryHeaps[i].flags == 1)
				Log::Print(LogLevel::LEVEL_INFO, "\tSize: %d mib  - Device Local", gpuMemoryProperties.memoryHeaps[i].size >> 20);
			else
				Log::Print(LogLevel::LEVEL_INFO, "\tSize: %d mib", gpuMemoryProperties.memoryHeaps[i].size >> 20);
		}

		Log::Print(LogLevel::LEVEL_INFO, "Memory types: %d", gpuMemoryProperties.memoryTypeCount);

		std::string str;
		for (uint32_t i = 0; i < gpuMemoryProperties.memoryTypeCount; i++)
		{
			if ((gpuMemoryProperties.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) == VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)
				str += "Device local, ";
			if ((gpuMemoryProperties.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) == VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)
				str += "Host visible, ";
			if ((gpuMemoryProperties.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) == VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)
				str += "Host coherent, ";
			if ((gpuMemoryProperties.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_HOST_CACHED_BIT) == VK_MEMORY_PROPERTY_HOST_CACHED_BIT)
				str += "Host cached";

			Log::Print(LogLevel::LEVEL_INFO, "\tHeap index: %d  - Property Flags: %s", gpuMemoryProperties.memoryTypes[i].heapIndex, str.c_str());
			str.clear();
		}

		return true;
	}

	bool VKBase::CreateLogicalDevice()
	{
		indices = vkutils::FindQueueFamilies(physicalDevice, surface);

		std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
		std::set<int> uniqueQueueFamilies = { indices.graphicsFamily, indices.presentFamily, indices.transferFamily };		// If the graphics family and the present family are the same, which is likely

		float queuePriority = 1.0f;

		for (int queueFamilyIndex : uniqueQueueFamilies)
		{
			VkDeviceQueueCreateInfo queueCreateInfo = {};
			queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			queueCreateInfo.queueFamilyIndex = queueFamilyIndex;
			queueCreateInfo.queueCount = 1;
			queueCreateInfo.pQueuePriorities = &queuePriority;

			queueCreateInfos.push_back(queueCreateInfo);
		}

		VkPhysicalDeviceFeatures deviceFeatures = {};
		deviceFeatures.textureCompressionBC = VK_TRUE;
		deviceFeatures.shaderClipDistance = VK_TRUE;
		deviceFeatures.wideLines = VK_TRUE;
		deviceFeatures.fragmentStoresAndAtomics = VK_TRUE;
		deviceFeatures.geometryShader = VK_TRUE;
		deviceFeatures.shaderStorageImageExtendedFormats = VK_TRUE;

		VkDeviceCreateInfo deviceInfo = {};
		deviceInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		deviceInfo.pQueueCreateInfos = queueCreateInfos.data();
		deviceInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
		deviceInfo.pEnabledFeatures = &deviceFeatures;
		deviceInfo.enabledExtensionCount = deviceExtensions.size();			// We check for support at physical device selection
		deviceInfo.ppEnabledExtensionNames = deviceExtensions.data();

		if (enableValidationLayers)
		{
			deviceInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
			deviceInfo.ppEnabledLayerNames = validationLayers.data();
		}
		else
		{
			deviceInfo.enabledLayerCount = 0;
		}

		if (vkCreateDevice(physicalDevice, &deviceInfo, nullptr, &device) != VK_SUCCESS)
		{
			Log::Print(LogLevel::LEVEL_ERROR, "Failed to create Logical Device");
			return false;
		}

		// QUEUES
		// No need to destroy the queues because it is implicitly destroyed when the logical device is destroyed
		vkGetDeviceQueue(device, indices.graphicsFamily, 0, &graphicsQueue);
		vkGetDeviceQueue(device, indices.presentFamily, 0, &presentQueue);
		vkGetDeviceQueue(device, indices.transferFamily, 0, &transferQueue);
		vkGetDeviceQueue(device, indices.computeFamily, 0, &computeQueue);

		return true;
	}

	bool VKBase::CreateCommandPool()
	{
		// Command Pool
		VkCommandPoolCreateInfo poolInfo = {};
		poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		poolInfo.queueFamilyIndex = indices.graphicsFamily;
		poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;			// Command buffers can be rerecorded. Possible flags are VK_COMMAND_POOL_CREATE_TRANSIENT_BIT and VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT

		if (vkCreateCommandPool(device, &poolInfo, nullptr, &cmdPool) != VK_SUCCESS)
		{
			Log::Print(LogLevel::LEVEL_ERROR, "Failed to create Command Pool!");
			return false;
		}

		poolInfo.queueFamilyIndex = indices.computeFamily;

		if (vkCreateCommandPool(device, &poolInfo, nullptr, &computeCmdPool) != VK_SUCCESS)
		{
			Log::Print(LogLevel::LEVEL_ERROR, "Failed to create compute command pool");
			return false;
		}

		return true;
	}
}
