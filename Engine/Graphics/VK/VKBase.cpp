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

		instance = VK_NULL_HANDLE;
		debugCallback = VK_NULL_HANDLE;
		physicalDevice = VK_NULL_HANDLE;
		device = VK_NULL_HANDLE;
		surface = VK_NULL_HANDLE;

		graphicsCmdPool = VK_NULL_HANDLE;
		computeCmdPool = VK_NULL_HANDLE;

		graphicsQueue = VK_NULL_HANDLE;
		presentQueue = VK_NULL_HANDLE;
		transferQueue = VK_NULL_HANDLE;
		computeQueue = VK_NULL_HANDLE;

		showAvailableExtensions = false;

		deviceFeatures = {};
		deviceProperties = {};
		gpuMemoryProperties = {};

		singleTimeCmdBuffer = {};
	}

	VKBase::~VKBase()
	{
	}

	bool VKBase::Init(GLFWwindow *window)
	{
		if (enableValidationLayers && !vkutils::ValidationLayersSupported(validationLayers))
		{
			Log::Print(LogLevel::LEVEL_ERROR, "Validation layers were requested, but are not available!\n");
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
		if (!CreateDevice())
			return false;
		if (!CreateGraphicsCommandPool())
			return false;
		if (!CreateComputeCommandPool())
			return false;

		vkAllocator = new VKAllocator(this);

		return true;
	}

	void VKBase::Dispose()
	{
		if(graphicsCmdPool != VK_NULL_HANDLE)
			vkDestroyCommandPool(device, graphicsCmdPool, nullptr);
		if (computeCmdPool != VK_NULL_HANDLE)
			vkDestroyCommandPool(device, computeCmdPool, nullptr);
		if (device != VK_NULL_HANDLE)
			vkDestroyDevice(device, nullptr);
		if (surface != VK_NULL_HANDLE)
			vkDestroySurfaceKHR(instance, surface, nullptr);
		if (debugCallback != VK_NULL_HANDLE)
			vkdebug::DestroyDebugReportCallbackEXT(instance, debugCallback, nullptr);
		if (instance != VK_NULL_HANDLE)
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
		allocInfo.commandPool = graphicsCmdPool;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandBufferCount = 1;

		VkCommandBuffer commandBuffer;
		if (vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer) != VK_SUCCESS)
		{
			Log::Print(LogLevel::LEVEL_ERROR, "Failed to allocate command buffer!\n");
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
			Log::Print(LogLevel::LEVEL_ERROR, "Failed to allocate compute command buffer!\n");
		}

		return cmdBuffer;
	}

	std::vector<VkCommandBuffer> VKBase::AllocateCommandBuffers(size_t count)
	{
		std::vector<VkCommandBuffer> commandBuffers;
		commandBuffers.resize(count);			// Commands are recorded for each swap chain image

		VkCommandBufferAllocateInfo allocInfo = {};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.commandPool = graphicsCmdPool;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandBufferCount = (uint32_t)commandBuffers.size();

		if (vkAllocateCommandBuffers(device, &allocInfo, commandBuffers.data()) != VK_SUCCESS)
		{
			Log::Print(LogLevel::LEVEL_ERROR, "Failed to allocate command buffers!\n");
		}

		return commandBuffers;
	}

	void VKBase::FreeCommandBuffer(VkCommandBuffer commandBuffer)
	{
		vkFreeCommandBuffers(device, graphicsCmdPool, 1, &commandBuffer);
	}

	void VKBase::FreeCommandBuffers(const std::vector<VkCommandBuffer>& commandBuffers)
	{
		vkFreeCommandBuffers(device, graphicsCmdPool, static_cast<uint32_t>(commandBuffers.size()), commandBuffers.data());
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
			Log::Print(LogLevel::LEVEL_ERROR, "Unsupported layout transition!\n");
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
		barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
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
		else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_GENERAL)
		{
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			barrier.dstAccessMask = 0;

			sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
			destinationStage = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
		}
		else
		{
			Log::Print(LogLevel::LEVEL_ERROR, "Unsupported layout transition!\n");
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
			Log::Print(LogLevel::LEVEL_ERROR, "Unsupported layout transition!\n");
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
			Log::Print(LogLevel::LEVEL_ERROR, "Unsupported layout transition!\n");
		}

		vkCmdPipelineBarrier(transferCb, sourceStage, destinationStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);
	}

	void VKBase::ShowAvailableExtensions()
	{
		uint32_t extensionCount = 0;
		vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);

		std::vector<VkExtensionProperties> extensions(extensionCount);
		vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data());

		Log::Print(LogLevel::LEVEL_INFO, "Available Extensions:\n");
		for (const auto &extension : extensions)
		{
			Log::Print(LogLevel::LEVEL_INFO, "\t%s\n", extension.extensionName);
		}
	}

	bool VKBase::CreateInstance()
	{
		VkApplicationInfo appInfo = {};
		appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		appInfo.pApplicationName = "CedrusEngine";
		appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.pEngineName = "CedrusEngine";
		appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.apiVersion = VK_API_VERSION_1_0;

		const std::vector<const char*> requiredExtentions = vkutils::GetRequiredExtensions(enableValidationLayers);

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
			Log::Print(LogLevel::LEVEL_ERROR, "Failed to create Vulkan Instance!\n");
			return false;
		}

		if (showAvailableExtensions)
		{
			uint32_t extensionCount = 0;
			vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);

			std::vector<VkExtensionProperties> extensions(extensionCount);
			vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data());

			std::cout << "Available Extensions:\n";
			for (const auto& extension : extensions)
			{
				std::cout << '\t' << extension.extensionName << '\n';
			}
		}

		Log::Print(LogLevel::LEVEL_INFO, "Created instance\n");

		return true;
	}

	bool VKBase::CreateDebugReportCallback()
	{
		// SETUP DEBUG CALLBACK
		VkDebugReportCallbackCreateInfoEXT callbackInfo = {};
		callbackInfo.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
		callbackInfo.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT | VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT;
		callbackInfo.pfnCallback = vkdebug::DebugCallback;

		if (vkdebug::CreateDebugReportCallbackEXT(instance, &callbackInfo, nullptr, &debugCallback) != VK_SUCCESS)
		{
			Log::Print(LogLevel::LEVEL_ERROR, "Failed to create debug report callback!\n");
			return false;
		}
		Log::Print(LogLevel::LEVEL_INFO, "Created debug report callback\n");

		return true;
	}

	bool VKBase::CreateSurface(GLFWwindow *window)
	{
		if (glfwCreateWindowSurface(instance, window, nullptr, &surface) != VK_SUCCESS)
		{
			Log::Print(LogLevel::LEVEL_ERROR, "Failed to create window surface!\n");
			return false;
		}
		Log::Print(LogLevel::LEVEL_INFO, "Created surface\n");

		return true;
	}

	bool VKBase::CreatePhysicalDevice()
	{
		uint32_t deviceCount = 0;
		vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);

		if (deviceCount == 0)
		{
			Log::Print(LogLevel::LEVEL_ERROR, "Failed to find GPUs with Vulkan support!\n");
			return false;
		}

		std::vector<VkPhysicalDevice> physicalDevices(deviceCount);
		vkEnumeratePhysicalDevices(instance, &deviceCount, physicalDevices.data());

		physicalDevice = vkutils::ChoosePhysicalDevice(physicalDevices, deviceExtensions, surface);

		if (physicalDevice == VK_NULL_HANDLE)
		{
			Log::Print(LogLevel::LEVEL_ERROR, "Failed to find suitable physical device");
			return false;
		}

		vkGetPhysicalDeviceFeatures(physicalDevice, &deviceFeatures);
		vkGetPhysicalDeviceProperties(physicalDevice, &deviceProperties);
		vkGetPhysicalDeviceMemoryProperties(physicalDevice, &gpuMemoryProperties);

		Log::Print(LogLevel::LEVEL_INFO, "GPU: %s\n", deviceProperties.deviceName);
		Log::Print(LogLevel::LEVEL_INFO, "Non coherent atom size: %d\n", deviceProperties.limits.nonCoherentAtomSize);
		Log::Print(LogLevel::LEVEL_INFO, "Max allocations: %d\n", deviceProperties.limits.maxMemoryAllocationCount);
		Log::Print(LogLevel::LEVEL_INFO, "Memory heaps: %d\n", gpuMemoryProperties.memoryHeapCount);

		for (uint32_t i = 0; i < gpuMemoryProperties.memoryHeapCount; i++)
		{			
			if (gpuMemoryProperties.memoryHeaps[i].flags == VK_MEMORY_HEAP_DEVICE_LOCAL_BIT)
				Log::Print(LogLevel::LEVEL_INFO, "\tSize: %d mib  - Device Local\n", gpuMemoryProperties.memoryHeaps[i].size >> 20);
			else
				Log::Print(LogLevel::LEVEL_INFO, "\tSize: %d mib\n", gpuMemoryProperties.memoryHeaps[i].size >> 20);
		}

		Log::Print(LogLevel::LEVEL_INFO, "Memory types: %d\n", gpuMemoryProperties.memoryTypeCount);

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

			Log::Print(LogLevel::LEVEL_INFO, "\tHeap index: %d  - Property Flags: %s\n", gpuMemoryProperties.memoryTypes[i].heapIndex, str.c_str());
			str.clear();
		}

		return true;
	}

	bool VKBase::CreateDevice()
	{
		queueIndices = vkutils::FindQueueFamilies(physicalDevice, surface, false, false);

		std::cout << "Graphics queue index: " << queueIndices.graphicsFamilyIndex << '\n';
		std::cout << "Present queue index: " << queueIndices.presentFamilyIndex << '\n';
		std::cout << "Transfer queue index: " << queueIndices.transferFamilyIndex << '\n';
		std::cout << "Compute queue index: " << queueIndices.computeFamilyIndex << '\n';

		std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
		std::set<int> uniqueQueueFamilies = { queueIndices.graphicsFamilyIndex, queueIndices.presentFamilyIndex, queueIndices.transferFamilyIndex, queueIndices.computeFamilyIndex };

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
			Log::Print(LogLevel::LEVEL_ERROR, "Failed to create Device\n");
			return false;
		}
		Log::Print(LogLevel::LEVEL_INFO, "Created Device\n");

		vkGetDeviceQueue(device, queueIndices.graphicsFamilyIndex, 0, &graphicsQueue);
		vkGetDeviceQueue(device, queueIndices.presentFamilyIndex, 0, &presentQueue);
		vkGetDeviceQueue(device, queueIndices.transferFamilyIndex, 0, &transferQueue);
		vkGetDeviceQueue(device, queueIndices.computeFamilyIndex, 0, &computeQueue);

		return true;
	}

	bool VKBase::CreateGraphicsCommandPool()
	{
		VkCommandPoolCreateInfo cmdPoolCreateInfo = {};
		cmdPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		cmdPoolCreateInfo.queueFamilyIndex = queueIndices.graphicsFamilyIndex;
		cmdPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

		if (vkCreateCommandPool(device, &cmdPoolCreateInfo, nullptr, &graphicsCmdPool) != VK_SUCCESS)
		{
			Log::Print(LogLevel::LEVEL_ERROR, "Failed to create graphics command pool");
			return false;
		}
		Log::Print(LogLevel::LEVEL_INFO, "Graphics command pool created");

		return true;
	}

	bool VKBase::CreateComputeCommandPool()
	{
		VkCommandPoolCreateInfo cmdPoolCreateInfo = {};
		cmdPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		cmdPoolCreateInfo.queueFamilyIndex = queueIndices.computeFamilyIndex;
		cmdPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

		if (vkCreateCommandPool(device, &cmdPoolCreateInfo, nullptr, &computeCmdPool) != VK_SUCCESS)
		{
			Log::Print(LogLevel::LEVEL_ERROR, "Failed to create compute command pool\n");
			return false;
		}
		Log::Print(LogLevel::LEVEL_INFO, "Compute command pool created");

		return true;
	}
}
