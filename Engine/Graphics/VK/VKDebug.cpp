#include "VKDebug.h"

#include "Program/Log.h"

#include <iostream>

namespace Engine
{
	namespace vkdebug
	{
		VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT objType, uint64_t obj, size_t location, int32_t code, const char *layerPrefix, const char *msg, void *userData)
		{
			std::cerr << "Validation Layer: " << msg << '\n';

			return VK_FALSE;
		}

		VkResult CreateDebugReportCallbackEXT(VkInstance instance, const VkDebugReportCallbackCreateInfoEXT * pCreateInfo, const VkAllocationCallbacks * pAllocator, VkDebugReportCallbackEXT * pCallback)
		{
			auto func = (PFN_vkCreateDebugReportCallbackEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugReportCallbackEXT");

			if (func != nullptr)
				return func(instance, pCreateInfo, pAllocator, pCallback);
			else
				return VK_ERROR_EXTENSION_NOT_PRESENT;
		}

		void DestroyDebugReportCallbackEXT(VkInstance instance, VkDebugReportCallbackEXT callback, const VkAllocationCallbacks * pAllocator)
		{
			auto func = (PFN_vkDestroyDebugReportCallbackEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugReportCallbackEXT");

			if (func != nullptr)
				func(instance, callback, pAllocator);
		}
	}
}
