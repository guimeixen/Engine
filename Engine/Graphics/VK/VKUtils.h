#pragma once

#include "Graphics\MaterialInfo.h"
#include "Graphics\Texture.h"

#include <vulkan\vulkan.h>

#include <vector>

namespace Engine
{
	struct QueueFamilyIndices {
		int graphicsFamilyIndex = -1;
		int presentFamilyIndex = -1;
		int transferFamilyIndex = -1;
		int computeFamilyIndex = -1;

		bool IsComplete()
		{
			return graphicsFamilyIndex >= 0 && presentFamilyIndex >= 0 && transferFamilyIndex >= 0 && computeFamilyIndex >= 0;
		}
	};

	struct SwapChainSupportDetails {
		VkSurfaceCapabilitiesKHR capabilities;
		std::vector<VkSurfaceFormatKHR> formats;
		std::vector<VkPresentModeKHR> presentModes;
	};

	namespace vkutils
	{
		bool ValidationLayersSupported(const std::vector<const char*> &validationLayers);
		std::vector<const char*> GetRequiredExtensions(bool enableValidationLayers);

		QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR surface, bool tryFindTransferOnlyQueue, bool tryFindComputeOnlyQueue);
		SwapChainSupportDetails QuerySwapChainSupport(VkPhysicalDevice device, VkSurfaceKHR surface);
		VkPhysicalDevice ChoosePhysicalDevice(const std::vector<VkPhysicalDevice>& physicalDevices, const std::vector<const char*>& deviceExtensions, VkSurfaceKHR surface);
		bool CheckPhysicalDeviceExtensionSupport(VkPhysicalDevice physicalDevice, const std::vector<const char*>& deviceExtensions);

		// Swap chain support
		VkSurfaceFormatKHR ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR> &availableFormats);
		VkPresentModeKHR ChooseSwapPresentMode(const std::vector<VkPresentModeKHR> &availablePresentModes);
		VkExtent2D ChooseSwapExtent(const VkSurfaceCapabilitiesKHR &capabilities, uint32_t width, uint32_t height);

		// Buffer
		uint32_t FindMemoryType(VkPhysicalDevice physicalDevice, uint32_t typeFilter, VkMemoryPropertyFlags properties);

		// Depth buffer format
		VkFormat FindSupportedFormat(VkPhysicalDevice physicalDevice, const std::vector<VkFormat> &candidates, VkImageTiling tiling, VkFormatFeatureFlags features);
		bool HasStencilComponent(VkFormat format);

		VkBlendFactor GetVKBlendFactor(BlendFactor blendFactor);
		VkPrimitiveTopology GetVKTopology(Topology topology);
		VkCompareOp GetDepthFunc(const std::string &func);
		VkCullModeFlagBits GetCullMode(const std::string &mode);
		VkFrontFace GetFrontFace(const std::string &face);
		VkFilter GetFilter(TextureFilter filter);
		VkSamplerAddressMode GetAddressMode(TextureWrap wrap);

		VkFormat GetFormat(TextureInternalFormat format);

		// Initialize or pre-initialize vulkan structs
		namespace init
		{
			VkPipelineVertexInputStateCreateInfo VertexInput(uint32_t bindingDescriptionCount, VkVertexInputBindingDescription *pBindingDescriptions, uint32_t attribDescriptionCount, VkVertexInputAttributeDescription *pAttribDescriptions);
			VkPipelineInputAssemblyStateCreateInfo InputAssembly(VkPrimitiveTopology topology, VkPipelineInputAssemblyStateCreateFlags flags, VkBool32 primitiveRestartEnable = VK_FALSE);
			VkPipelineViewportStateCreateInfo Viewport(uint32_t viewportCount, uint32_t scissorCount, VkPipelineViewportStateCreateFlags flags);
			VkPipelineRasterizationStateCreateInfo Rasterization(VkPolygonMode polygonMode, VkCullModeFlags cullMode, VkFrontFace frontFace, VkPipelineRasterizationStateCreateFlags flags = 0);
			VkPipelineMultisampleStateCreateInfo Multisample(VkSampleCountFlagBits rasterizationSamples);
			VkPipelineColorBlendStateCreateInfo BlendState(uint32_t attachmentCount, const VkPipelineColorBlendAttachmentState *pAttachments);
			VkPipelineColorBlendAttachmentState BlendAttachment(VkBool32 enableBlending);
			VkPipelineDepthStencilStateCreateInfo DepthStencil(VkBool32 depthTestEnable, VkBool32 depthWriteEnable, VkCompareOp depthCompareOp);

			VkAttachmentDescription AttachmentDesc(VkFormat format, VkAttachmentLoadOp loadOp, VkAttachmentStoreOp storeOp, VkImageLayout finalLayout, VkImageLayout initialLayout = VK_IMAGE_LAYOUT_UNDEFINED);
			VkWriteDescriptorSet WriteDescSet(VkDescriptorSet dstSet, uint32_t dstBinding, VkDescriptorType type);
		}
	}
}
