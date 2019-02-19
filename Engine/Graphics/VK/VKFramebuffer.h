#pragma once

#include "VKTexture2D.h"
#include "Graphics\Framebuffer.h"

namespace Engine
{
	class VKFramebuffer : public Framebuffer
	{
	public:
		VKFramebuffer();
		VKFramebuffer(VKAllocator *allocator, VkPhysicalDevice physicalDevice, VkDevice device, const FramebufferDesc &desc);
		~VKFramebuffer();

		void Dispose();

		VkFramebuffer GetHandle() const { return framebuffer; }
		VkRenderPass GetRenderPass() const { return renderPass; }
		VkSemaphore GetSemaphore() const { return semaphore; }

		uint32_t GetClearValueCount() const { return static_cast<uint32_t>(clearValues.size()); }
		const VkClearValue *GetClearValues() const { return clearValues.data(); }

		bool IsDepthOnly() const { return isDepthOnly; }

		void Resize(const FramebufferDesc &desc) override;
		void Clear() const override {}
		void Bind() const override {}

	private:
		void Create(const FramebufferDesc &desc, bool createRenderPass);
		void CreateEmpty(const FramebufferDesc &desc, bool createRenderPass);		// Used when writes to the framebuffer are disabled
		void CreateSemaphore(VkDevice device);

	private:
		VKAllocator *allocator;
		VkPhysicalDevice physicalDevice;
		VkDevice device;
		VkFramebuffer framebuffer;
		VkRenderPass renderPass;
		VkSemaphore semaphore;

		std::vector<VkClearValue> clearValues;

		bool isDepthOnly;
	};
}
