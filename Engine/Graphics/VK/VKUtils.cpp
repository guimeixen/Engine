#include "VKUtils.h"

#include "Program\Log.h"

#include "include\GLFW\glfw3.h"

#include <set>
#include <algorithm>

namespace Engine
{
	namespace vkutils
	{
		bool ValidationLayersSupported(const std::vector<const char*>& validationLayers)
		{
			uint32_t layerPropCount = 0;
			vkEnumerateInstanceLayerProperties(&layerPropCount, nullptr);

			std::vector<VkLayerProperties> availableLayers(layerPropCount);
			vkEnumerateInstanceLayerProperties(&layerPropCount, availableLayers.data());

			// Compare all the layers we want against the available ones. If we didn't find return false
			for (const char *layerName : validationLayers)
			{
				bool layerFound = false;

				for (const auto &layer : availableLayers)
				{
					if (std::strcmp(layerName, layer.layerName) == 0)
					{
						layerFound = true;
						break;
					}
				}

				if (!layerFound)
					return false;
			}

			return true;
		}

		std::vector<const char*> GetRequiredExtensions(bool enableValidationLayers)
		{
			std::vector<const char*> extensions;

			uint32_t glfwExtensionCount = 0;
			const char **glfwExtensions;
			glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

			Log::Print(LogLevel::LEVEL_INFO, "Required GLFW extensions:\n");

			for (uint32_t i = 0; i < glfwExtensionCount; i++)
			{
				extensions.push_back(glfwExtensions[i]);
				Log::Print(LogLevel::LEVEL_INFO, "%s\n", glfwExtensions[i]);
			}

			if (enableValidationLayers)
				extensions.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);

			return extensions;
		}

		QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR surface)
		{
			QueueFamilyIndices indices;

			uint32_t queueFamilyCount = 0;
			vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

			std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
			vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

			Log::Print(LogLevel::LEVEL_INFO, "Queue families count: %d\n", queueFamilyCount);

			int i = 0;
			VkBool32 presentSupport = false;

			for (const auto &queueFamily : queueFamilies)
			{
				if (queueFamily.queueCount > 0 && queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
				{
					indices.graphicsFamily = i;
				}

				// Choose the first queue
				if (indices.transferFamily == -1 && queueFamily.queueCount > 0 && queueFamily.queueFlags & VK_QUEUE_TRANSFER_BIT /*&& (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) == 0*/)		// Try using an exclusive queue for transfer
				{
					indices.transferFamily = i;
				}

				// TODO: If no exclusive compute queue found then find the first that supports compute
				if (queueFamily.queueCount > 0 && queueFamily.queueFlags & VK_QUEUE_COMPUTE_BIT && (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) == 0)		// Find a compute only queue
				{
					indices.computeFamily = i;
				}

				vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);

				if (queueFamily.queueCount > 0 && presentSupport)
					indices.presentFamily = i;

				if (indices.IsComplete())
					break;

				i++;
			}

			// If we didn't find a compute exlusive queue, then find the first one that supports compute
			if (indices.computeFamily == -1)
			{
				for (size_t i = 0; i < queueFamilies.size(); i++)
				{
					if (queueFamilies[i].queueCount > 0 && queueFamilies[i].queueFlags & VK_QUEUE_COMPUTE_BIT)
					{
						indices.computeFamily = i;
						break;
					}
				}
			}

			return indices;
		}

		bool CheckDeviceExtensionSupported(VkPhysicalDevice device, const std::vector<const char*> &deviceExtensions)
		{
			uint32_t extensionCount = 0;
			vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

			std::vector<VkExtensionProperties> availableExtensions(extensionCount);
			vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

			std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());

			for (const auto& extension : availableExtensions)
			{
				requiredExtensions.erase(extension.extensionName);
			}

			return requiredExtensions.empty();
		}

		SwapChainSupportDetails QuerySwapChainSupport(VkPhysicalDevice device, VkSurfaceKHR surface)
		{
			SwapChainSupportDetails details;

			// Surface capabilities
			vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);

			// Surface formats
			uint32_t formatCount;
			vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);

			if (formatCount != 0)
			{
				details.formats.resize(formatCount);
				vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.formats.data());
			}

			// Surface present modes
			uint32_t presentModeCount;
			vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);

			if (presentModeCount != 0)
			{
				details.presentModes.resize(presentModeCount);
				vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, details.presentModes.data());
			}

			return details;
		}

		bool IsPhysicalDeviceSuitable(VkPhysicalDevice device, VkSurfaceKHR surface, const std::vector<const char*>& deviceExtensions)
		{
			VkPhysicalDeviceProperties deviceProperties;
			vkGetPhysicalDeviceProperties(device, &deviceProperties);

			VkPhysicalDeviceFeatures deviceFeatures;
			vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

			QueueFamilyIndices indices = FindQueueFamilies(device, surface);

			bool extensionsSupported = CheckDeviceExtensionSupported(device, deviceExtensions);

			bool swapChainApropriate = false;
			if (extensionsSupported)
			{
				SwapChainSupportDetails swapChainSupport = QuerySwapChainSupport(device, surface);
				swapChainApropriate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
			}

			bool featuresAvailable = false;

			if (deviceFeatures.textureCompressionBC && deviceFeatures.shaderClipDistance && deviceFeatures.wideLines && deviceFeatures.fragmentStoresAndAtomics && deviceFeatures.geometryShader && deviceFeatures.shaderStorageImageExtendedFormats)
				featuresAvailable = true;

			//return deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU;
			return indices.IsComplete() && extensionsSupported && swapChainApropriate && featuresAvailable;
		}

		VkSurfaceFormatKHR ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR> &availableFormats)
		{
			// If the surface has no preferred format then choose the one we want
			if (availableFormats.size() == 1 && availableFormats[0].format == VK_FORMAT_UNDEFINED)
			{
				return{ VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR };
			}

			// Else check if the format we want is in the list of available formats
			for (const auto& availableFormat : availableFormats)
			{
				if (availableFormat.format == VK_FORMAT_B8G8R8A8_UNORM && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
				{
					return availableFormat;
				}
			}

			// If that fails we could rank the format based on how good they are
			// but in most cases it's ok to settle with the first one
			return availableFormats[0];
		}

		VkPresentModeKHR ChooseSwapPresentMode(const std::vector<VkPresentModeKHR> &availablePresentModes)
		{
			VkPresentModeKHR bestMode = VK_PRESENT_MODE_FIFO_KHR;

			for (const auto& availablePresentMode : availablePresentModes)
			{
				if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR)
				{
					return availablePresentMode;
				}
				else if (availablePresentMode == VK_PRESENT_MODE_IMMEDIATE_KHR)		// Some drivers don't support FIFO so we should prefer Immediate if Mailbox is not available
				{
					bestMode = availablePresentMode;
				}
			}

			return bestMode;
		}

		VkExtent2D ChooseSwapExtent(const VkSurfaceCapabilitiesKHR &capabilities, uint32_t width, uint32_t height)
		{
			if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
			{
				return capabilities.currentExtent;
			}
			else
			{
				VkExtent2D actualExtent = { width, height };

				actualExtent.width = std::max(capabilities.minImageExtent.width, std::min(capabilities.maxImageExtent.width, actualExtent.width));
				actualExtent.height = std::max(capabilities.minImageExtent.height, std::min(capabilities.maxImageExtent.height, actualExtent.height));

				return actualExtent;
			}
		}

		uint32_t FindMemoryType(VkPhysicalDevice physicalDevice, uint32_t typeFilter, VkMemoryPropertyFlags properties)
		{
			VkPhysicalDeviceMemoryProperties memProps;
			vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProps);

			for (uint32_t i = 0; i < memProps.memoryTypeCount; i++)
			{
				if (typeFilter & (1 << i) && (memProps.memoryTypes[i].propertyFlags & properties) == properties)		// Check if the bit is set to 1
				{
					return i;
				}
			}

			Log::Print(LogLevel::LEVEL_ERROR, "Failed to find memory type!\n");
			return 0;
		}

		VkFormat FindSupportedFormat(VkPhysicalDevice physicalDevice, const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features)
		{
			for (VkFormat format : candidates)
			{
				VkFormatProperties properties;
				vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &properties);

				if (tiling == VK_IMAGE_TILING_LINEAR && (properties.linearTilingFeatures & features) == features)
				{
					return format;
				}
				else if (tiling == VK_IMAGE_TILING_OPTIMAL && (properties.optimalTilingFeatures & features) == features)
				{
					return format;
				}
			}

			assert(0);

			return candidates[0];
		}

		bool HasStencilComponent(VkFormat format)
		{
			return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
		}

		VkBlendFactor GetVKBlendFactor(BlendFactor blendFactor)
		{
			switch (blendFactor)
			{
			case ZERO:
				return VK_BLEND_FACTOR_ZERO;
				break;
			case ONE:
				return VK_BLEND_FACTOR_ONE;
				break;
			case SRC_ALPHA:
				return VK_BLEND_FACTOR_SRC_ALPHA;
				break;
			case DST_ALPHA:
				return VK_BLEND_FACTOR_DST_ALPHA;
				break;
			case SRC_COLOR:
				return VK_BLEND_FACTOR_SRC_COLOR;
				break;
			case DST_COLOR:
				return VK_BLEND_FACTOR_DST_COLOR;
				break;
			case ONE_MINUS_SRC_ALPHA:
				return VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
				break;
			case ONE_MINUS_SRC_COLOR:
				return VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR;
				break;
			}

			return VK_BLEND_FACTOR_ZERO;
		}

		VkPrimitiveTopology GetVKTopology(Topology topology)
		{
			switch (topology)
			{
			case TRIANGLES:
				return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
				break;
			case TRIANGLE_STRIP:
				return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
				break;
			case LINES:
				return VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
				break;
			case LINE_TRIP:
				return VK_PRIMITIVE_TOPOLOGY_LINE_STRIP;
				break;
			default:
				break;
			}

			return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		}

		VkCompareOp GetDepthFunc(const std::string &func)
		{
			if (func == "less")
				return VK_COMPARE_OP_LESS;
			else if (func == "lequal")
				return VK_COMPARE_OP_LESS_OR_EQUAL;
			else if (func == "greater")
				return VK_COMPARE_OP_GREATER;
			else if (func == "gequal")
				return VK_COMPARE_OP_GREATER_OR_EQUAL;
			else if (func == "nequal")
				return VK_COMPARE_OP_NOT_EQUAL;
			else if (func == "equal")
				return VK_COMPARE_OP_EQUAL;
			else if (func == "never")
				return VK_COMPARE_OP_NEVER;
			else if (func == "always")
				return VK_COMPARE_OP_ALWAYS;

			return VK_COMPARE_OP_LESS;
		}

		VkCullModeFlagBits GetCullMode(const std::string &mode)
		{
			if (mode == "front")
				return VK_CULL_MODE_FRONT_BIT;
			else if (mode == "back")
				return VK_CULL_MODE_BACK_BIT;
			else if (mode == "none")
				return VK_CULL_MODE_NONE;

			return VK_CULL_MODE_BACK_BIT;
		}

		VkFrontFace GetFrontFace(const std::string &face)
		{
			if (face == "ccw")
				return VK_FRONT_FACE_COUNTER_CLOCKWISE;
			else if (face == "cw")
				return VK_FRONT_FACE_CLOCKWISE;

			return VK_FRONT_FACE_COUNTER_CLOCKWISE;
		}

		VkFilter GetFilter(TextureFilter filter)
		{
			switch (filter)
			{
			case TextureFilter::LINEAR:
				return VK_FILTER_LINEAR;
				break;
			case TextureFilter::NEAREST:
				return VK_FILTER_NEAREST;
				break;
			}
			return VK_FILTER_LINEAR;
		}

		VkSamplerAddressMode GetAddressMode(TextureWrap wrap)
		{
			switch (wrap)
			{
			case TextureWrap::REPEAT:
				return VK_SAMPLER_ADDRESS_MODE_REPEAT;
				break;
			case TextureWrap::CLAMP:
				return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
				break;
			case TextureWrap::MIRRORED_REPEAT:
				return VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
				break;
			case TextureWrap::CLAMP_TO_EDGE:
				return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
				break;
			case TextureWrap::CLAMP_TO_BORDER:
				return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
				break;
			}

			return VK_SAMPLER_ADDRESS_MODE_REPEAT;
		}

		VkFormat GetFormat(TextureInternalFormat format)
		{
			switch (format)
			{
			case TextureInternalFormat::RGB8:
				return VK_FORMAT_R8G8B8_UNORM;
				break;
			case TextureInternalFormat::RGBA8:
				return VK_FORMAT_R8G8B8A8_UNORM;
				break;
			case TextureInternalFormat::RGB16F:
				return VK_FORMAT_R16G16B16_SFLOAT;
				break;
			case TextureInternalFormat::RGBA16F:
				return VK_FORMAT_R16G16B16A16_SFLOAT;
				break;
			case TextureInternalFormat::SRGB8:
				return VK_FORMAT_R8G8B8_SRGB;
				break;
			case TextureInternalFormat::SRGB8_ALPHA8:
				return VK_FORMAT_R8G8B8A8_SRGB;
				break;
			case TextureInternalFormat::DEPTH_COMPONENT16:
				return VK_FORMAT_D16_UNORM;
				break;
			case TextureInternalFormat::DEPTH_COMPONENT24:
				return VK_FORMAT_D24_UNORM_S8_UINT;
				break;
			case TextureInternalFormat::DEPTH_COMPONENT32:
				return VK_FORMAT_D32_SFLOAT;
				break;
			case TextureInternalFormat::RED8:
				return VK_FORMAT_R8_UNORM;
				break;
			case TextureInternalFormat::R16F:
				return VK_FORMAT_R16_SFLOAT;
				break;
			case TextureInternalFormat::R32UI:
				return VK_FORMAT_R32_UINT;
				break;
			case TextureInternalFormat::RG32UI:
				return VK_FORMAT_R32G32_UINT;
			case TextureInternalFormat::RG8:
				return VK_FORMAT_R8G8_UNORM;
				break;
			}

			return VK_FORMAT_UNDEFINED;
		}

		namespace init
		{
			VkPipelineVertexInputStateCreateInfo VertexInput(uint32_t bindingDescriptionCount, VkVertexInputBindingDescription * pBindingDescriptions, uint32_t attribDescriptionCount, VkVertexInputAttributeDescription * pAttribDescriptions)
			{
				VkPipelineVertexInputStateCreateInfo info = {};
				info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
				info.vertexBindingDescriptionCount = bindingDescriptionCount;
				info.pVertexBindingDescriptions = pBindingDescriptions;
				info.vertexAttributeDescriptionCount = attribDescriptionCount;
				info.pVertexAttributeDescriptions = pAttribDescriptions;

				return info;
			}
			VkPipelineInputAssemblyStateCreateInfo InputAssembly(VkPrimitiveTopology topology, VkPipelineInputAssemblyStateCreateFlags flags, VkBool32 primitiveRestartEnable)
			{
				VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo = {};
				inputAssemblyInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
				inputAssemblyInfo.topology = topology;
				inputAssemblyInfo.flags = flags;
				inputAssemblyInfo.primitiveRestartEnable = primitiveRestartEnable;

				return inputAssemblyInfo;
			}

			VkPipelineViewportStateCreateInfo Viewport(uint32_t viewportCount, uint32_t scissorCount, VkPipelineViewportStateCreateFlags flags)
			{
				VkPipelineViewportStateCreateInfo viewportStateInfo = {};
				viewportStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
				viewportStateInfo.viewportCount = 1;
				viewportStateInfo.scissorCount = 1;
				viewportStateInfo.flags = flags;

				return viewportStateInfo;
			}

			VkPipelineRasterizationStateCreateInfo Rasterization(VkPolygonMode polygonMode, VkCullModeFlags cullMode, VkFrontFace frontFace, VkPipelineRasterizationStateCreateFlags flags)
			{
				VkPipelineRasterizationStateCreateInfo rasterizationStateInfo = {};
				rasterizationStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
				rasterizationStateInfo.flags = flags;
				rasterizationStateInfo.depthClampEnable = VK_FALSE;				// if true, fragments beyond the near and far plane are clamped to it instead of discarding them. Useful for shadow maps. Requires enabling a GPU feature
				rasterizationStateInfo.rasterizerDiscardEnable = VK_FALSE;		// If true, geometry never passes through the rasterizer. This disables output to the framebuffer
				rasterizationStateInfo.polygonMode = polygonMode;		// Any mode other than FILL like LINE for wireframe requires enabling a GPU feature
				rasterizationStateInfo.lineWidth = 1.0f;						// Line thickness in fragments. More than 1 requires enabling wideLines GPU feature
				rasterizationStateInfo.cullMode = cullMode;
				rasterizationStateInfo.frontFace = frontFace;
				rasterizationStateInfo.depthBiasEnable = VK_FALSE;					// The rasterizer can alter the depth values by adding a constant value or biasing them based on a fragment's slope. Can be used for shadow mapping

				return rasterizationStateInfo;
			}

			VkPipelineMultisampleStateCreateInfo Multisample(VkSampleCountFlagBits rasterizationSamples)
			{
				VkPipelineMultisampleStateCreateInfo multisampleInfo = {};
				multisampleInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
				multisampleInfo.sampleShadingEnable = VK_FALSE;
				multisampleInfo.rasterizationSamples = rasterizationSamples;
				multisampleInfo.minSampleShading = 1.0f;				// Optional
				multisampleInfo.pSampleMask = nullptr;					// Optional
				multisampleInfo.alphaToCoverageEnable = VK_FALSE;		// Optional
				multisampleInfo.alphaToOneEnable = VK_FALSE;			// Optional

				return multisampleInfo;
			}

			VkPipelineColorBlendStateCreateInfo BlendState(uint32_t attachmentCount, const VkPipelineColorBlendAttachmentState *pAttachments)
			{
				VkPipelineColorBlendStateCreateInfo colorBlendingInfo = {};
				colorBlendingInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
				colorBlendingInfo.logicOpEnable = VK_FALSE;
				colorBlendingInfo.logicOp = VK_LOGIC_OP_COPY;				// Optional, other then copy requires enabling GPU feature
				colorBlendingInfo.attachmentCount = attachmentCount;
				colorBlendingInfo.pAttachments = pAttachments;
				colorBlendingInfo.blendConstants[0] = 0.0f;					// Optional
				colorBlendingInfo.blendConstants[1] = 0.0f;					// Optional
				colorBlendingInfo.blendConstants[2] = 0.0f;					// Optional
				colorBlendingInfo.blendConstants[3] = 0.0f;					// Optional

				return colorBlendingInfo;
			}
			VkPipelineColorBlendAttachmentState BlendAttachment(VkBool32 enableBlending)
			{
				VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
				colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
				colorBlendAttachment.blendEnable = enableBlending;
				colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
				colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
				colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
				colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
				colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
				colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

				return colorBlendAttachment;
			}
			VkPipelineDepthStencilStateCreateInfo DepthStencil(VkBool32 depthTestEnable, VkBool32 depthWriteEnable, VkCompareOp depthCompareOp)
			{
				VkPipelineDepthStencilStateCreateInfo info = {};
				info.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
				info.depthTestEnable = depthTestEnable;
				info.depthWriteEnable = depthWriteEnable;
				info.depthCompareOp = depthCompareOp;
				info.depthBoundsTestEnable = VK_FALSE;			// Used to only keep fragments that fall within the depth range below
				info.minDepthBounds = 0.0f;		// Optional
				info.maxDepthBounds = 1.0f;		// Optional
				info.stencilTestEnable = VK_FALSE;
				info.front = {};
				info.back = {};

				return info;
			}

			VkAttachmentDescription AttachmentDesc(VkFormat format, VkAttachmentLoadOp loadOp, VkAttachmentStoreOp storeOp, VkImageLayout finalLayout, VkImageLayout initialLayout)
			{
				VkAttachmentDescription attach = {};
				attach.format = format;
				attach.samples = VK_SAMPLE_COUNT_1_BIT;
				attach.loadOp = loadOp;
				attach.storeOp = storeOp;
				attach.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
				attach.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
				attach.initialLayout = initialLayout;
				attach.finalLayout = finalLayout;

				return attach;
			}

			VkWriteDescriptorSet WriteDescSet(VkDescriptorSet dstSet, uint32_t dstBinding, VkDescriptorType type)
			{
				VkWriteDescriptorSet write = {};
				write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				write.dstSet = dstSet;
				write.dstBinding = dstBinding;
				write.dstArrayElement = 0;
				write.descriptorType = type;
				write.descriptorCount = 1;

				return write;
			}
		}
	}
}
