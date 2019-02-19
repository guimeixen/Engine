#include "VKFramebuffer.h"

#include "VKUtils.h"
#include "Program\Utils.h"
#include "Graphics\Texture.h"

#include <iostream>
#include <array>

namespace Engine
{
	VKFramebuffer::VKFramebuffer()
	{
		depthAttachment.texture = nullptr;
		isDepthOnly = false;
	}

	VKFramebuffer::VKFramebuffer(VKAllocator *allocator, VkPhysicalDevice physicalDevice, VkDevice device, const FramebufferDesc &desc)
	{
		framebuffer = VK_NULL_HANDLE;
		renderPass = VK_NULL_HANDLE;
		this->physicalDevice = physicalDevice;
		this->device = device;
		this->allocator = allocator;	
		this->passID = desc.passID;
		width = desc.width;
		height = desc.height;
		isDepthOnly = desc.colorTextures.size() == 0;
		colorOnly = !isDepthOnly && !desc.useDepth;

		depthAttachment.params = {};
		depthAttachment.texture = nullptr;

		if (desc.writesDisabled)
			CreateEmpty(desc, true);
		else
			Create(desc, true);

		CreateSemaphore(device);
	}

	VKFramebuffer::~VKFramebuffer()
	{
	}

	void VKFramebuffer::Dispose()
	{
		for (size_t i = 0; i < colorAttachments.size(); i++)
		{
			colorAttachments[i].texture->RemoveReference();
		}
		colorAttachments.clear();

		if (depthAttachment.texture)
		{
			depthAttachment.texture->RemoveReference();
		}
		vkDestroyFramebuffer(device, framebuffer, nullptr);
		vkDestroyRenderPass(device, renderPass, nullptr);
		vkDestroySemaphore(device, semaphore, nullptr);
	}

	void VKFramebuffer::Resize(const FramebufferDesc &desc)
	{
		if (desc.passID != passID)
		{
			std::cout << "Error: Trying to resize framebuffer whose passID don't match\n";
			return;
		}
		if (desc.useDepth && depthAttachment.texture == nullptr)
		{
			std::cout << "Error: Trying to resize framebuffer with no depth attachment but now it wants one\n";
			return;
		}
		if (desc.colorTextures.size() != colorAttachments.size())
		{
			std::cout << "Error: Trying to resize framebuffer with a different number of color attachments\n";
			return;
		}

		if (desc.width == width && desc.height == height)
			return;

		if (framebuffer != VK_NULL_HANDLE)
		{
			for (size_t i = 0; i < colorAttachments.size(); i++)
			{
				colorAttachments[i].texture->RemoveReference();
			}
			colorAttachments.clear();

			if (depthAttachment.texture)
			{
				depthAttachment.texture->RemoveReference();
			}
			vkDestroyFramebuffer(device, framebuffer, nullptr);
		}

		width = desc.width;
		height = desc.height;

		if (desc.writesDisabled)
			CreateEmpty(desc, false);
		else
			Create(desc, false);		// No need to recreate render pass
	}

	void VKFramebuffer::Create(const FramebufferDesc &desc, bool createRenderPass)
	{
		std::vector<VkAttachmentDescription> attachmentsDescs;
		std::vector<VkAttachmentReference> colorAttachmentsRefs;
		VkAttachmentReference depthRef = {};

		if (isDepthOnly && desc.useDepth)
		{
			clearValues.resize(1);
			clearValues[0].depthStencil = { 1.0f, 0 };

			VKTexture2D *depthStencilAttachment = new VKTexture2D();
			depthStencilAttachment->CreateDepthStencilAttachment(allocator, physicalDevice, device, width, height, desc.depthTexture, desc.sampleDepth, false);
			depthStencilAttachment->AddReference();
			depthAttachment.texture = depthStencilAttachment;
			depthAttachment.params = desc.depthTexture;

			VkAttachmentDescription attachDesc = {};
			attachDesc.format = depthStencilAttachment->GetFormat();
			attachDesc.samples = VK_SAMPLE_COUNT_1_BIT;
			attachDesc.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
			attachDesc.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			attachDesc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			attachDesc.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;						// We don't care about initial layout of the attachment

			if (desc.sampleDepth)
			{
				attachDesc.storeOp = VK_ATTACHMENT_STORE_OP_STORE;							// We will read from depth, so it's important to store the depth attachment results
				attachDesc.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;	// Attachment will be transitioned to shader read at render pass end
			}
			else
			{
				attachDesc.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
				attachDesc.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
			}

			attachmentsDescs.push_back(attachDesc);

			depthRef.attachment = 0;
			depthRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;				// Attachment will be used as depth/stencil during render pass
		}
		else
		{
			if (desc.useDepth)
			{
				VKTexture2D *depthStencilAttachment = new VKTexture2D();
				depthStencilAttachment->CreateDepthStencilAttachment(allocator, physicalDevice, device, width, height, desc.depthTexture, desc.sampleDepth, false);
				depthStencilAttachment->AddReference();
				depthAttachment.texture = depthStencilAttachment;
				depthAttachment.params = desc.depthTexture;

				size_t size = desc.colorTextures.size() + 1;
				attachmentsDescs.resize(size);
				clearValues.resize(size);
				colorAttachmentsRefs.resize(size - 1);

				clearValues[clearValues.size() - 1].depthStencil = { 1.0f, 0 };

				// Depth attachment
				VkAttachmentDescription &attachDesc = attachmentsDescs[attachmentsDescs.size() - 1];
				attachDesc.format = depthStencilAttachment->GetFormat();
				attachDesc.samples = VK_SAMPLE_COUNT_1_BIT;
				attachDesc.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
				attachDesc.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
				attachDesc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
				attachDesc.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;						// We don't care about initial layout of the attachment

				if (desc.sampleDepth)
				{
					attachDesc.storeOp = VK_ATTACHMENT_STORE_OP_STORE;							// We will read from depth, so it's important to store the depth attachment results
					attachDesc.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;	// Attachment will be transitioned to shader read at render pass end
				}
				else
				{
					attachDesc.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
					attachDesc.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
				}

				depthRef.attachment = desc.colorTextures.size();
				depthRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
			}
			else
			{
				attachmentsDescs.resize(desc.colorTextures.size());
				colorAttachmentsRefs.resize(desc.colorTextures.size());
				clearValues.resize(desc.colorTextures.size());
			}


			for (size_t i = 0; i < desc.colorTextures.size(); i++)
			{
				const TextureParams &colorParams = desc.colorTextures[i];

				clearValues[i].color = { 0.0f, 0.0f, 0.0f, 1.0f };

				VKTexture2D *colorAttachment = new VKTexture2D();
				colorAttachment->CreateColorAttachment(allocator, physicalDevice, device, width, height, colorParams);

				FramebufferAttachment attachment = {};
				attachment.texture = colorAttachment;
				attachment.params = colorParams;

				colorAttachments.push_back(attachment);

				VkAttachmentDescription &attachDesc = attachmentsDescs[i];
				attachDesc.format = colorAttachment->GetFormat();
				attachDesc.samples = VK_SAMPLE_COUNT_1_BIT;
				attachDesc.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;			// Parse the clear from the desc
				attachDesc.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
				attachDesc.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
				attachDesc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
				attachDesc.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
				attachDesc.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

				VkAttachmentReference &ref = colorAttachmentsRefs[i];
				ref.attachment = static_cast<uint32_t>(i);
				ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
			}
		}

		// Subpasses describe how an attachment will be treated during its execution
		VkSubpassDescription subpassDesc = {};
		subpassDesc.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;

		if (isDepthOnly)
		{
			subpassDesc.pDepthStencilAttachment = &depthRef;		// Depth attachment reference is stored as last
		}
		else
		{
			subpassDesc.colorAttachmentCount = static_cast<uint32_t>(colorAttachmentsRefs.size());
			subpassDesc.pColorAttachments = colorAttachmentsRefs.data();
			if (desc.useDepth)
				subpassDesc.pDepthStencilAttachment = &depthRef;
		}

		// Dependencies describe the execution order between subpasses
		// They also describe the equivalent of a pipeline barrier between two subpasses, or between a subpass and the outside of the whole render pass (vk_subpass_external)
		// Use subpass dependencies for layout transitions
		VkSubpassDependency dependencies[2] = {};

		// First dependency at the start of the renderpass
		// Does the transition from final to initial layout 
		dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
		dependencies[0].dstSubpass = 0;
		dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

		// Second dependency at the end the renderpass
		// Does the transition from the initial to the final layout
		dependencies[1].srcSubpass = 0;
		dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
		dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

		if ((desc.sampleDepth && !isDepthOnly) || desc.colorTextures.size() > 0)
		{
			dependencies[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;											// Stage that produces data. Only execute the subpass when the previous pipeline invocation has finished.
			dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;									// Stage that consumes data. Subpass starts execution at this stage
			dependencies[0].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;														// Method of access that produces data
			dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;		// Method of access that consumes data

			dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
			dependencies[1].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
			dependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
			dependencies[1].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
		}
		else if (desc.sampleDepth)
		{
			dependencies[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
			dependencies[0].dstStageMask = VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
			dependencies[0].srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
			dependencies[0].dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

			dependencies[1].srcStageMask = VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
			dependencies[1].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
			dependencies[1].srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
			dependencies[1].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
		}

		// Create the render pass
		VkRenderPassCreateInfo passInfo = {};
		passInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		passInfo.attachmentCount = static_cast<uint32_t>(attachmentsDescs.size());
		passInfo.pAttachments = attachmentsDescs.data();
		passInfo.subpassCount = 1;
		passInfo.pSubpasses = &subpassDesc;
		passInfo.dependencyCount = 2;
		passInfo.pDependencies = dependencies;

		// Render pass only needs to be built one time
		if (createRenderPass && vkCreateRenderPass(device, &passInfo, nullptr, &renderPass) != VK_SUCCESS)
		{
			std::cout << "Error! Failed to create render pass\n";
		}

		std::vector<VkImageView> attachments(attachmentsDescs.size());
		for (size_t i = 0; i < attachments.size(); i++)
		{
			if (desc.useDepth && i == attachments.size() - 1)		// Depth attachment
			{
				attachments[i] = static_cast<VKTexture2D*>(depthAttachment.texture)->GetImageView();
			}
			else
			{
				attachments[i] = static_cast<VKTexture2D*>(colorAttachments[i].texture)->GetImageView();
			}
		}

		VkFramebufferCreateInfo fbInfo = {};
		fbInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		fbInfo.renderPass = renderPass;
		fbInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
		fbInfo.pAttachments = attachments.data();
		fbInfo.width = width;
		fbInfo.height = height;
		fbInfo.layers = 1;

		if (vkCreateFramebuffer(device, &fbInfo, nullptr, &framebuffer) != VK_SUCCESS)
		{
			std::cout << "Error! Failed to create render pass\n";
		}
	}

	void VKFramebuffer::CreateEmpty(const FramebufferDesc &desc, bool createRenderPass)
	{
		// Subpasses describe how an attachment will be treated during its execution
		VkSubpassDescription subpassDesc = {};
		subpassDesc.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;

		// Create the render pass
		VkRenderPassCreateInfo passInfo = {};
		passInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		passInfo.attachmentCount = 0;
		passInfo.pAttachments = nullptr;
		passInfo.subpassCount = 1;
		passInfo.pSubpasses = &subpassDesc;
		passInfo.dependencyCount = 0;
		passInfo.pDependencies = nullptr;

		// Render pass only needs to be built one time
		if (createRenderPass && vkCreateRenderPass(device, &passInfo, nullptr, &renderPass) != VK_SUCCESS)
		{
			std::cout << "Error! Failed to create render pass\n";
		}

		VkFramebufferCreateInfo fbInfo = {};
		fbInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		fbInfo.renderPass = renderPass;
		fbInfo.attachmentCount = 0;
		fbInfo.pAttachments = nullptr;
		fbInfo.width = width;
		fbInfo.height = height;
		fbInfo.layers = 1;

		if (vkCreateFramebuffer(device, &fbInfo, nullptr, &framebuffer) != VK_SUCCESS)
		{
			std::cout << "Error! Failed to create render pass\n";
		}
	}

	void VKFramebuffer::CreateSemaphore(VkDevice device)
	{
		VkSemaphoreCreateInfo semaphoreCreateInfo = {};
		semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

		if (vkCreateSemaphore(device, &semaphoreCreateInfo, nullptr, &semaphore) != VK_SUCCESS)
			std::cout << "Failed to create semaphore!\n";
	}
}
