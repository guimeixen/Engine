#include "VKRenderer.h"

#include "Program/Log.h"
#include "VKTexture2D.h"
#include "VKTexture3D.h"
#include "VKTextureCube.h"
#include "VKShader.h"
#include "Graphics/Mesh.h"
#include "Graphics/Material.h"
#include "Graphics/VertexArray.h"
#include "Graphics/ResourcesLoader.h"
#include "VKBuffer.h"
#include "VKVertexArray.h"

#include "Program/Utils.h"
#include "Program/StringID.h"

#include "Data/Shaders/bindings.glsl"

#include "include/glm/gtc/matrix_transform.hpp"
#include "include/glm/gtc/type_ptr.hpp"

#include <array>
#include <iostream>

namespace Engine
{
	VKRenderer::VKRenderer(FileManager *fileManager, GLFWwindow *window, unsigned int width, unsigned int height, unsigned int monitorWidth, unsigned int monitorHeight)
	{
		this->fileManager = fileManager;
		this->window = window;
		this->width = width;
		this->height = height;
		this->monitorWidth = monitorWidth;
		this->monitorHeight = monitorHeight;

		imageIndex = 0;

		currentAPI = GraphicsAPI::Vulkan;
		descriptorPool = VK_NULL_HANDLE;
		defaultRenderPass = VK_NULL_HANDLE;
		curPipeline = VK_NULL_HANDLE;
		userSetLayout = VK_NULL_HANDLE;
		globalTexturesSet = VK_NULL_HANDLE;
		globalTexturesSetLayout = VK_NULL_HANDLE;
		globalBuffersSetLayout = VK_NULL_HANDLE;
		graphicsPipelineLayout = VK_NULL_HANDLE;
		transferCommandBuffer = VK_NULL_HANDLE;
		backBufferRenderPassInfos = {};	
		currentFB = nullptr;
		instanceDataSSBO = nullptr;
		mappedInstanceData = nullptr;
		instanceDataOffset = 0;
		instanceDataBufferSingleSize = 0;
		currentFrame = 0;
		currentCamera = 0;
		cameraUBOData = nullptr;
		cameraUBO = nullptr;
		singleCameraAlignedSize = 0;
		allCamerasAlignedSize = 0;
		framesWaitedToRemove = 0;

		for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
		{
			frameResources[i] = {};
		}
	}

	VKRenderer::~VKRenderer()
	{
		Dispose();
	}

	bool VKRenderer::Init()
	{
		if (!base.Init(window))
			return false;

		base.ShowAvailableExtensions();

		VkPhysicalDevice physicalDevice = base.GetPhysicalDevice();
		VkDevice device = base.GetDevice();
		VkSurfaceKHR surface = base.GetSurface();

		std::cout << "Max push constant size: " << base.GetDeviceLimits().maxPushConstantsSize << '\n';

		swapChain.Init(base.GetAllocator(), physicalDevice, surface, device, width, height);

		if (!CreateDefaultRenderPass())
			return false;

		// Swap chain framebuffers created here because we need the render pass
		swapChain.CreateFramebuffers(device, defaultRenderPass);

		transferCommandBuffer = base.AllocateCommandBuffer();		// If using dedicated transfer queue, create another command pool

		VkCommandBufferBeginInfo beginInfo = {};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
		vkBeginCommandBuffer(transferCommandBuffer, &beginInfo);

		// Transtion the layout of the swap chain depth texture
		base.TransitionImageLayout(transferCommandBuffer, swapChain.GetDepthTexture(), VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, swapChain.GetDepthTexture()->GetMipLevels());

		VkSemaphoreCreateInfo semaphoreInfo = {};
		semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

		VkFenceCreateInfo fenceInfo = {};
		fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

		for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
		{
			if (vkCreateSemaphore(device, &semaphoreInfo, nullptr, &frameResources[i].imageAvailableSemaphore) != VK_SUCCESS)
			{
				Log::Print(LogLevel::LEVEL_ERROR, "Failed to create semaphore\n");
				return false;
			}
			if (vkCreateSemaphore(device, &semaphoreInfo, nullptr, &frameResources[i].renderFinishedSemaphore) != VK_SUCCESS)
			{
				Log::Print(LogLevel::LEVEL_ERROR, "Failed to create semaphore\n");
				return false;
			}
			if (vkCreateFence(device, &fenceInfo, nullptr, &frameResources[i].frameFence) != VK_SUCCESS)
			{
				Log::Print(LogLevel::LEVEL_ERROR, "Failed to create fence\n");
				return false;
			}
			/*if (vkCreateFence(device, &fenceInfo, nullptr, &frameResources[i].computeFence) != VK_SUCCESS)
			{
				Log::Print(LogLevel::LEVEL_ERROR, "Failed to create compute fence\n");
				return false;
			}*/

			frameResources[i].frameCmdBuffer = base.AllocateCommandBuffer();
			//frameResources[i].computeCmdBuffer = base.AllocateComputeCommandBuffer();
		}

		VkDescriptorSetLayoutBinding cameraLayoutBinding = {};
		cameraLayoutBinding.binding = CAMERA_UBO;
		cameraLayoutBinding.descriptorCount = 1;
		cameraLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
		cameraLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_GEOMETRY_BIT | VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT;

		VkDescriptorSetLayoutBinding instanceDataBinding = {};
		instanceDataBinding.binding = INSTANCE_DATA_SSBO;
		instanceDataBinding.descriptorCount = 1;
		instanceDataBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		instanceDataBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

		globalBuffersSetLayoutBindings.push_back(cameraLayoutBinding);
		globalBuffersSetLayoutBindings.push_back(instanceDataBinding);

		// Dynamic ubo for cameras
		unsigned int minUBOAlignment = (unsigned int)base.GetDeviceLimits().minUniformBufferOffsetAlignment;
		singleCameraAlignedSize = utils::Align(sizeof(CameraUBO), minUBOAlignment);
		allCamerasAlignedSize = utils::Align(sizeof(CameraUBO) * MAX_CAMERAS, minUBOAlignment);

		instanceDataBufferSingleSize = 1024 * 512;

		// The buffer creation takes care of alignment
		cameraUBO = new VKBuffer(&base, nullptr, singleCameraAlignedSize * MAX_CAMERAS * MAX_FRAMES_IN_FLIGHT, BufferType::UniformBuffer, BufferUsage::DYNAMIC);
		instanceDataSSBO = new VKBuffer(&base, nullptr, instanceDataBufferSingleSize * MAX_FRAMES_IN_FLIGHT, BufferType::ShaderStorageBuffer, BufferUsage::DYNAMIC);				// 512 kib	

		cameraUBOData = (glm::mat4*)malloc(size_t(cameraUBO->GetAlignedSize()));

		Log::Print(LogLevel::LEVEL_INFO, "Min ubo offset alignment: %u\n", minUBOAlignment);

		VkDescriptorPoolSize poolSize[5] = {};
		poolSize[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		poolSize[0].descriptorCount = 9;

		poolSize[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		poolSize[1].descriptorCount = 350;									// Good value? How to choose ? Right now is a random value

		poolSize[2].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
		poolSize[2].descriptorCount = 2;

		poolSize[3].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		poolSize[3].descriptorCount = 7;

		poolSize[4].type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
		poolSize[4].descriptorCount = 15;

		VkDescriptorPoolCreateInfo poolInfo = {};
		poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		poolInfo.poolSizeCount = 5;
		poolInfo.pPoolSizes = poolSize;
		poolInfo.maxSets = 310;

		if (vkCreateDescriptorPool(device, &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS)
		{
			Log::Print(LogLevel::LEVEL_ERROR, "Failed to create descriptor pool\n");
			return false;
		}

		VkWriteDescriptorSet cameraUBOWrite = {};
		cameraUBOWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		cameraUBOWrite.descriptorCount = 1;
		cameraUBOWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
		cameraUBOWrite.dstBinding = CAMERA_UBO;

		VkWriteDescriptorSet instanceDataWrite = {};
		instanceDataWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		instanceDataWrite.descriptorCount = 1;
		instanceDataWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		instanceDataWrite.dstBinding = INSTANCE_DATA_SSBO;

		VKBufferInfo cameraUBOInfo = {};
		cameraUBOInfo.binding = CAMERA_UBO;
		cameraUBOInfo.buffer = cameraUBO;

		VKBufferInfo instanceBufInfo = {};
		instanceBufInfo.binding = INSTANCE_DATA_SSBO;
		instanceBufInfo.buffer = instanceDataSSBO;

		globalBuffersSetWrites.push_back(cameraUBOWrite);
		globalBuffersSetWrites.push_back(instanceDataWrite);

		bufferInfos.push_back(cameraUBOInfo);
		bufferInfos.push_back(instanceBufInfo);

		return true;
	}

	bool VKRenderer::PostLoad(ScriptManager &scriptManager)
	{
		VertexInputDesc desc = {};
		desc.attribs.push_back({VertexAttributeFormat::FLOAT, 2, 0});
		desc.stride = 2 * sizeof(float);

		defaultMaterial = CreateMaterialInstanceFromBaseMat(scriptManager, "Data/Materials/default_mat.lua", { desc });

		if (!defaultMaterial)
			return false;

		base.GetAllocator()->PrintStats();
		Log::Print(LogLevel::LEVEL_INFO, "Total pipelines: %u\n", pipelines.size());

		return true;
	}

	VertexArray *VKRenderer::CreateVertexArray(const VertexInputDesc &desc, Buffer *vertexBuffer, Buffer *indexBuffer)
	{
		return new VKVertexArray(desc, vertexBuffer, indexBuffer);
	}

	VertexArray *VKRenderer::CreateVertexArray(const VertexInputDesc *descs, unsigned int descCount, const std::vector<Buffer*> &vertexBuffers, Buffer *indexBuffer)
	{
		return new VKVertexArray(descs, descCount, vertexBuffers, indexBuffer);
	}

	Buffer *VKRenderer::CreateVertexBuffer(const void *data, unsigned int size, BufferUsage usage)
	{
		VKBuffer *vb = new VKBuffer(&base, data, size, BufferType::VertexBuffer, usage);
		vb->AddReference();
		vertexBuffers.push_back(vb);

		if (vb->GetStagingBuffer() != VK_NULL_HANDLE)
		{
			//base.CopyBuffer(vb->GetStagingBuffer(), vb->GetBuffer(), vb->GetSize());
			VkBufferCopy copyRegion = {};
			copyRegion.srcOffset = 0;
			copyRegion.dstOffset = 0;
			copyRegion.size = size;					// Size must be the actual used size, not the aligned size which is what vb->GetSize() returns
			vkCmdCopyBuffer(transferCommandBuffer, vb->GetStagingBuffer(), vb->GetBuffer(), 1, &copyRegion);
		}

		needsTransfers = true;

		return vb;
	}

	Buffer *VKRenderer::CreateIndexBuffer(const void *data, unsigned int size, BufferUsage usage)
	{
		//VKIndexBuffer *ib = new VKIndexBuffer(&base, data, size, usage);
		VKBuffer* ib = new VKBuffer(&base, data, size, BufferType::IndexBuffer, usage);
		ib->AddReference();
		indexBuffers.push_back(ib);

		if (ib->GetStagingBuffer() != VK_NULL_HANDLE)
		{
			//base.CopyBuffer(ib->GetStagingBuffer(), ib->GetBuffer(), ib->GetSize());
			VkBufferCopy copyRegion = {};
			copyRegion.srcOffset = 0;
			copyRegion.dstOffset = 0;
			copyRegion.size = size;
			vkCmdCopyBuffer(transferCommandBuffer, ib->GetStagingBuffer(), ib->GetBuffer(), 1, &copyRegion);
		}

		needsTransfers = true;

		return ib;
	}

	Buffer *VKRenderer::CreateUniformBuffer(const void *data, unsigned int size)
	{
		unsigned int alignedSize = utils::Align(size, static_cast<unsigned int>(base.GetDeviceLimits().minUniformBufferOffsetAlignment));

		VKBuffer* ubo = new VKBuffer(&base, data, alignedSize * MAX_FRAMES_IN_FLIGHT, BufferType::UniformBuffer, BufferUsage::DYNAMIC);
		ubo->AddReference();
		ubos.push_back(ubo);
		return ubo;
	}

	Buffer *VKRenderer::CreateDrawIndirectBuffer(unsigned int size, const void *data)
	{
		//VKDrawIndirectBuffer *buf = new VKDrawIndirectBuffer(&base, data, size);
		VKBuffer* buf = new VKBuffer(&base, data, size, BufferType::DrawIndirectBuffer, BufferUsage::DYNAMIC);
		buf->AddReference();
		drawIndirectBufs.push_back(buf);
		return buf;
	}

	Buffer *VKRenderer::CreateSSBO(unsigned int size, const void *data, unsigned int stride, BufferUsage usage)
	{
		//VKSSBO *ssbo = new VKSSBO(&base, data, size, usage);
		VKBuffer* ssbo = new VKBuffer(&base, data, size, BufferType::ShaderStorageBuffer, usage);
		ssbo->AddReference();
		ssbos.push_back(ssbo);
		return ssbo;
	}

	Framebuffer *VKRenderer::CreateFramebuffer(const FramebufferDesc &desc)
	{
		VKFramebuffer *fb = new VKFramebuffer(base.GetAllocator(), base.GetPhysicalDevice(), base.GetDevice(), desc);
		fb->AddReference();
		framebuffers.push_back(fb);
		return fb;
	}

	ShaderProgram* VKRenderer::CreateShader(const std::string &vertexName, const std::string &fragmentName, const std::string &defines, const std::vector<VertexInputDesc> &descs, const BlendState &blendState)
	{
		unsigned int id = SID(vertexName + fragmentName + defines);

		// If the shader already exists return that one instead
		if (shaderPrograms.find(id) != shaderPrograms.end())
		{
			return shaderPrograms[id];
		}

		// Else create a new one
		ShaderProgram *shader = new VKShader(id, vertexName, fragmentName, defines);
		shaderPrograms[id] = shader;

		return shader;
	}

	ShaderProgram* VKRenderer::CreateShader(const std::string &vertexName, const std::string &fragmentName, const std::vector<VertexInputDesc> &descs, const BlendState &blendState)
	{
		return CreateShader(vertexName, fragmentName, descs, blendState);
	}

	ShaderProgram* VKRenderer::CreateShaderWithGeometry(const std::string &vertexPath, const std::string &geometryPath, const std::string &fragmentPath, const std::string &defines, const std::vector<VertexInputDesc> &descs)
	{
		unsigned int id = SID(vertexPath + geometryPath + fragmentPath + defines);

		// If the shader already exists return that one instead
		if (shaderPrograms.find(id) != shaderPrograms.end())
		{
			return shaderPrograms[id];
		}

		// Else create a new one
		ShaderProgram* shader = new VKShader(id, vertexPath, geometryPath, fragmentPath, defines);
		shaderPrograms[id] = shader;

		return shader;
	}

	ShaderProgram* VKRenderer::CreateShaderWithGeometry(const std::string &vertexPath, const std::string &geometryPath, const std::string &fragmentPath, const std::vector<VertexInputDesc> &descs)
	{
		return CreateShaderWithGeometry(vertexPath, geometryPath, fragmentPath, "", descs);
	}

	ShaderProgram* VKRenderer::CreateComputeShader(const std::string &computePath, const std::string &defines)
	{
		unsigned int id = SID(computePath + defines);

		// If the shader already exists return that one instead
		if (shaderPrograms.find(id) != shaderPrograms.end())
		{
			return shaderPrograms[id];
		}

		// Else create a new one
		ShaderProgram* shader = new VKShader(id, computePath, defines);
		shaderPrograms[id] = shader;

		return shader;
	}

	ShaderProgram* VKRenderer::CreateComputeShader(const std::string &computePath)
	{
		return CreateComputeShader(computePath, "");
	}

	MaterialInstance *VKRenderer::CreateMaterialInstance(ScriptManager &scriptManager, const std::string &matInstPath, const std::vector<VertexInputDesc> &inputDescs)
	{
		size_t oldMatSize = ResourcesLoader::GetMaterials().size();

		MaterialInstance *m = Material::LoadMaterialInstance(this, matInstPath, scriptManager, inputDescs);
		materialInstances.push_back(m);

		const std::map<unsigned int, MaterialRefInfo> &materials = ResourcesLoader::GetMaterials();
		size_t newMatSize = materials.size();

		// Check if we loaded a completely new material so that we create the pipeline
		if (newMatSize > oldMatSize)
		{
			std::vector<ShaderPass> &shaderPasses = m->baseMaterial->GetShaderPasses();

			for (size_t i = 0; i < shaderPasses.size(); i++)
			{
				ShaderPass &p = shaderPasses[i];
				
				if (!CreatePipeline(p, m))
					return defaultMaterial;
				
				p.pipelineID = static_cast<unsigned int>(pipelines.size() - 1);
			}
		}

		// Check if this material instance doesn't have a set
		/*if (m->setID > sets.size())		// The initial value of setID is uint_max
		{*/
			CreateSetForMaterialInstance(m, PipelineType::GRAPHICS);
			UpdateMaterialInstance(m);
		//}

		return m;
	}

	MaterialInstance *VKRenderer::CreateMaterialInstanceFromBaseMat(ScriptManager &scriptManager, const std::string &baseMatPath, const std::vector<VertexInputDesc> &inputDescs)
	{
		const std::map<unsigned int, MaterialRefInfo>& materials = ResourcesLoader::GetMaterials();

		size_t oldMatSize = materials.size();

		MaterialInstance *m = Material::LoadMaterialInstanceFromBaseMat(this, baseMatPath, scriptManager, inputDescs);
		materialInstances.push_back(m);
	
		size_t newMatSize = materials.size();

		PipelineType type = PipelineType::GRAPHICS;

		// Check if we loaded a completely new material so that we create a new pipeline
		if (newMatSize > oldMatSize)
		{
			std::vector<ShaderPass> &shaderPasses = m->baseMaterial->GetShaderPasses();

			for (size_t i = 0; i < shaderPasses.size(); i++)		// TODO: Only allow one pipeline per compute material?
			{
				ShaderPass& p = shaderPasses[i];

				if (!CreatePipeline(p, m))
					return defaultMaterial;

				p.pipelineID = static_cast<unsigned int>(pipelines.size() - 1);

				if (p.isCompute)
				{
					type = PipelineType::COMPUTE;
					CreateSetForMaterialInstance(m, type);				
				}
			}
		}
		
		// TODO: Don't allow a material instance to have more than one pass if it is compute
		m->computePipelineLayoutIdx = static_cast<unsigned int>(computePipelineLayouts.size() - 1);

		
		if (type == PipelineType::GRAPHICS && m->graphicsSetID > sets.size())		// The initial value of setID is uint_max
		{
			CreateSetForMaterialInstance(m, type);
		}

		return m;
	}

	Texture *VKRenderer::CreateTexture2D(const std::string &path, const TextureParams &params, bool storeTextureData)
	{
		unsigned int id = SID(path);
		
		// If the texture already exists return that one instead
		if (textures.find(id) != textures.end())
		{
			Texture *tex = textures[id];
			tex->AddReference();
			//std::cout << path << '\n';
			return tex;
		}

		VKTexture2D *tex = new VKTexture2D(&base, path, params, storeTextureData);
		if (!tex->Load(&base))
			return nullptr;

		tex->AddReference();
		
		textures[id] = tex;

		PrepareTexture2D(tex);
		needsTransfers = true;

		return tex;
	}

	Texture *VKRenderer::CreateTexture3D(const std::string &path, const void *data, unsigned int width, unsigned int height, unsigned int depth, const TextureParams &params)
	{
		unsigned int id = SID(path);

		// If the texture already exists return that one instead
		if (textures.find(id) != textures.end())
		{
			Texture *tex = textures[id];
			tex->AddReference();
			return textures[id];
		}

		return nullptr;
	}

	Texture *VKRenderer::CreateTextureCube(const std::vector<std::string> &faces, const TextureParams &params)
	{
		unsigned int id = SID(faces[0]);		// Better way? Otherwise if faces[0] is equal to an already stored texture but the other faces are different it won't get loaded

		// If the texture already exists return that one instead
		if (textures.find(id) != textures.end())
		{
			Texture *tex = textures[id];
			tex->AddReference();
			return textures[id];
		}

		VKTextureCube *tex = new VKTextureCube(faces, params);
		tex->AddReference();
		tex->Load(&base);
		textures[id] = tex;

		base.TransitionImageLayout(transferCommandBuffer, tex, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
		const std::vector<VkBufferImageCopy> &copyRegions = tex->GetCopyRegions();
		vkCmdCopyBufferToImage(transferCommandBuffer, tex->GetStagingBuffer(), tex->GetImage(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, static_cast<uint32_t>(copyRegions.size()), copyRegions.data());
		base.TransitionImageLayout(transferCommandBuffer, tex, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);			// To be able to sample the texture in the shader, we need one last transition to prepare it for shader access

		tex->CreateImageView(base.GetDevice());
		tex->CreateSampler(base.GetDevice());

		needsTransfers = true;

		return tex;
	}

	Texture *VKRenderer::CreateTextureCube(const std::string &path, const TextureParams &params)
	{
		unsigned int id = SID(path);

		// If the texture already exists return that one instead
		if (textures.find(id) != textures.end())
		{
			Texture *tex = textures[id];
			tex->AddReference();
			return textures[id];
		}

		VKTextureCube *tex = new VKTextureCube(path, params);
		tex->AddReference();
		tex->Load(&base);
		textures[id] = tex;

		base.TransitionImageLayout(transferCommandBuffer, tex, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
		const std::vector<VkBufferImageCopy> &copyRegions = tex->GetCopyRegions();
		vkCmdCopyBufferToImage(transferCommandBuffer, tex->GetStagingBuffer(), tex->GetImage(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, static_cast<uint32_t>(copyRegions.size()), copyRegions.data());
		base.TransitionImageLayout(transferCommandBuffer, tex, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);			// To be able to sample the texture in the shader, we need one last transition to prepare it for shader access

		tex->CreateImageView(base.GetDevice());
		tex->CreateSampler(base.GetDevice());

		needsTransfers = true;

		return tex;
	}

	Texture *VKRenderer::CreateTexture2DFromData(unsigned int width, unsigned int height, const TextureParams &params, const void *data)
	{
		// We use a increasing id for textures that are loaded from data because we don't have a path string to convert to ID.
		// So loop until a find a free ID
		while (textures.find(textureID) != textures.end())
		{
			//std::cout << "Conflicting texture ID's\n";
			textureID++;
		}

		VKTexture2D *tex = new VKTexture2D(&base, width, height, params, data);
		tex->AddReference();
		textures[textureID] = tex;
		textureID++;

		PrepareTexture2D(tex);

		needsTransfers = true;

		return tex;
	}

	Texture *VKRenderer::CreateTexture3DFromData(unsigned int width, unsigned int height, unsigned int depth, const TextureParams &params, const void *data)
	{
		while (textures.find(textureID) != textures.end())
		{
			textureID++;
		}

		VKTexture3D *tex = new VKTexture3D(&base, width, height, depth, params, data);
		tex->AddReference();
		textures[textureID] = tex;
		textureID++;

		if (tex->GetStagingBuffer() == VK_NULL_HANDLE && data)		// Support for 3d textures not complete
		{
			std::cout << "3D texture staging buffer is null\n";
			return nullptr;
		}

		PrepareTexture3D(tex);

		return tex;
	}

	void VKRenderer::Resize(unsigned int width, unsigned int height)
	{
		if (this->width = width && this->height == height)
			return;

		VkDevice device = base.GetDevice();
		vkDeviceWaitIdle(device);

		std::cout << "Resizing Vulkan swap chain\n";

		this->width = width;
		this->height = height;

		swapChain.Recreate(base.GetAllocator(), base.GetPhysicalDevice(), base.GetSurface(), device, static_cast<uint32_t>(width), static_cast<uint32_t>(height));
		CreateDefaultRenderPass();
		swapChain.CreateFramebuffers(device, defaultRenderPass);

		wasResized = true;
	}

	void VKRenderer::SetCamera(Camera *camera, const glm::vec4 &clipPlane)
	{
		this->camera = camera;

		// Check for camera limits here
		if (currentCamera >= MAX_CAMERAS)
		{
			Log::Print(LogLevel::LEVEL_ERROR, "Too many cameras!\n");
			return;
		}
	
		CameraUBO *ubo = (CameraUBO*)(((uint64_t)cameraUBOData + (currentCamera * singleCameraAlignedSize)));
		ubo->proj = camera->GetProjectionMatrix();
		ubo->proj[1][1] *= -1;
		ubo->proj[3][1] *= -1;		// For orthographic cameras
		ubo->view = camera->GetViewMatrix();
		ubo->projView = ubo->proj * ubo->view;
		ubo->invView = glm::inverse(ubo->view);
		ubo->invProj = glm::inverse(ubo->proj);
		ubo->clipPlane = clipPlane;
		ubo->camPos = glm::vec4(camera->GetPosition(), 0.0f);
		ubo->nearFarPlane = glm::vec2(camera->GetNearPlane(), camera->GetFarPlane());

		uint32_t dynamicOffset = static_cast<uint32_t>(currentCamera) * singleCameraAlignedSize;
		vkCmdBindDescriptorSets(frameResources[currentFrame].frameCmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipelineLayout, 0, 1, &frameResources[currentFrame].globalBuffersSet, 1, &dynamicOffset);

		currentCamera++;
	}

	void VKRenderer::UpdateBuffer(Buffer* ubo, const void* data, unsigned int size, unsigned int offset)
	{
		VKBuffer* b = static_cast<VKBuffer*>(ubo);
		b->Update(data, size, currentFrame * (b->GetAlignedSize() / MAX_FRAMES_IN_FLIGHT));
	}

	void VKRenderer::SetDefaultRenderTarget()
	{
		useDefaultFramebuffer = true;

		curPipeline = VK_NULL_HANDLE;

		VkClearValue clearValues[2];
		clearValues[0].color = { 0.0f, 0.0f, 0.0f, 1.0f };
		clearValues[1].depthStencil = { 1.0f, 0 };

		VkExtent2D extent = swapChain.GetExtent();

		VkRenderPassBeginInfo renderPassBeginInfo = {};
		renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassBeginInfo.renderPass = defaultRenderPass;
		renderPassBeginInfo.renderArea.offset = { 0, 0 };
		renderPassBeginInfo.renderArea.extent = extent;
		renderPassBeginInfo.clearValueCount = 2;
		renderPassBeginInfo.pClearValues = clearValues;
		renderPassBeginInfo.framebuffer = swapChain.GetFramebuffer(currentFrame);

		VkViewport vkviewport = {};
		vkviewport.x = 0.0f;
		vkviewport.y = 0.0f;
		vkviewport.width = (float)extent.width;
		vkviewport.height = (float)extent.height;
		vkviewport.minDepth = 0.0f;
		vkviewport.maxDepth = 1.0f;

		const VkCommandBuffer &cb = frameResources[currentFrame].frameCmdBuffer;
		vkCmdBeginRenderPass(cb, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
		vkCmdSetViewport(cb, 0, 1, &vkviewport);
	}

	void VKRenderer::SetRenderTarget(Framebuffer *rt)
	{
	}

	void VKRenderer::SetRenderTargetAndClear(Framebuffer *rt)
	{
		currentFB = static_cast<VKFramebuffer*>(rt);

		useDefaultFramebuffer = false;

		VkRenderPassBeginInfo renderPassBeginInfo = {};
		renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassBeginInfo.renderPass = currentFB->GetRenderPass();
		renderPassBeginInfo.renderArea.offset = { 0, 0 };
		renderPassBeginInfo.renderArea.extent.width = currentFB->GetWidth();
		renderPassBeginInfo.renderArea.extent.height = currentFB->GetHeight();
		renderPassBeginInfo.framebuffer = currentFB->GetHandle();
		renderPassBeginInfo.clearValueCount = currentFB->GetClearValueCount();
		renderPassBeginInfo.pClearValues = currentFB->GetClearValues();

		VkViewport vkviewport = {};
		vkviewport.x = 0.0f;
		vkviewport.y = 0.0f;
		vkviewport.width = (float)renderPassBeginInfo.renderArea.extent.width;
		vkviewport.height = (float)renderPassBeginInfo.renderArea.extent.height;
		vkviewport.minDepth = 0.0f;
		vkviewport.maxDepth = 1.0f;

		VkCommandBuffer cmdBuffer = frameResources[currentFrame].frameCmdBuffer;

		vkCmdBeginRenderPass(cmdBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
		vkCmdSetViewport(cmdBuffer, 0, 1, &vkviewport);
	}

	void VKRenderer::EndRenderTarget(Framebuffer *rt)
	{
		vkCmdEndRenderPass(frameResources[currentFrame].frameCmdBuffer);
	}

	void VKRenderer::EndDefaultRenderTarget()
	{
		const VkCommandBuffer &cb = frameResources[currentFrame].frameCmdBuffer;
		vkCmdEndRenderPass(cb);

		/*if (vkEndCommandBuffer(cb) != VK_SUCCESS)
			Log::Print(LogLevel::LEVEL_ERROR, "Failed to record command buffer\n");*/
	}

	void VKRenderer::ClearRenderTarget(Framebuffer *rt)
	{
	}

	void VKRenderer::SetViewport(const Viewport &viewport)
	{
		VkViewport vkviewport = {};
		vkviewport.x = (float)viewport.x;
		vkviewport.y = (float)viewport.y;
		vkviewport.width = (float)viewport.width;
		vkviewport.height = (float)viewport.height;
		vkviewport.minDepth = 0.0f;
		vkviewport.maxDepth = 1.0f;

		vkCmdSetViewport(frameResources[currentFrame].frameCmdBuffer, 0, 1, &vkviewport);
	}

	void VKRenderer::Submit(const RenderQueue &renderQueue)
	{
		for (size_t i = 0; i < renderQueue.size(); i++)
		{
			// Copy the transform and the mesh data
			const RenderItem &ri = renderQueue[i];

			if (ri.transform)
			{
				//mapped = (char*)ri.transform;
				memcpy(mappedInstanceData, ri.transform, sizeof(glm::mat4));
				mappedInstanceData += 64;
			}
			if (ri.instanceData)
			{
				//mapped = (char*)ri.instanceData;
				memcpy(mappedInstanceData, ri.instanceData, ri.instanceDataSize);
				mappedInstanceData += ri.instanceDataSize;
			}
			if (ri.meshParams)
			{
				memcpy(mappedInstanceData, ri.meshParams, ri.meshParamsSize);
				mappedInstanceData += ri.meshParamsSize;
			}

			Submit(ri);
		}	
	}

	void VKRenderer::Submit(const RenderItem &renderItem)
	{
		const std::vector<Buffer*> &vbs = renderItem.mesh->vao->GetVertexBuffers();
		VKBuffer* ib = static_cast<VKBuffer*>(renderItem.mesh->vao->GetIndexBuffer());

		std::vector<VkBuffer> vertexBuffers(vbs.size());
		std::vector<VkDeviceSize> vbOffsets(vbs.size());
		for (size_t i = 0; i < vertexBuffers.size(); i++)
		{
			vertexBuffers[i] = static_cast<VKBuffer*>(vbs[i])->GetBuffer();
			vbOffsets[i] = 0;
		}

		const ShaderPass &pass = renderItem.matInstance->baseMaterial->GetShaderPass(renderItem.shaderPass);
		VkPipeline pipeline = pipelines[pass.pipelineID];
		const VkCommandBuffer &cb = frameResources[currentFrame].frameCmdBuffer;

		//if (pipeline != curPipeline)
		vkCmdBindPipeline(cb, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);		// TODO: Sort pipelines
		vkCmdBindVertexBuffers(cb, 0, vertexBuffers.size(), vertexBuffers.data(), vbOffsets.data());

		// Check the set to see if it's equal to the previous so we don't bind it
		if (renderItem.matInstance->textures.size() > 0)
			vkCmdBindDescriptorSets(cb, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipelineLayout, 2, 1, &sets[renderItem.matInstance->graphicsSetID].set[currentFrame], 0, nullptr);

		struct data
		{
			unsigned int startIndex;
			unsigned int numVecs;
		};

		if (renderItem.transform)
		{			
			data d = {};
			d.startIndex = instanceDataOffset;
			d.numVecs = 4;

			// Check what's faster, two push constants calls or memcpy'ing first and one push constant call

			vkCmdPushConstants(cb, graphicsPipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, 8, &d);

			if (renderItem.materialDataSize > 0)
				vkCmdPushConstants(cb, graphicsPipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 16, renderItem.materialDataSize, renderItem.materialData);				// Offset of 16 because of padding

			//instanceDataOffset += 4;			// If we're treating the data as vec4 instead of mat4
			instanceDataOffset += 1;
		}
		else if (renderItem.instanceData)
		{
			data d = {};
			d.startIndex = instanceDataOffset;
			d.numVecs = 4;

			// Check what's faster, two push constants calls or memcpy'ing first and one push constant call

			vkCmdPushConstants(cb, graphicsPipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, 8, &d);

			if (renderItem.materialDataSize > 0)
				vkCmdPushConstants(cb, graphicsPipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 16, renderItem.materialDataSize, renderItem.materialData);				// Offset of 16 because of padding
																																															//instanceDataOffset += 4;			// If we're treating the data as vec4 instead of mat4
			instanceDataOffset += renderItem.instanceDataSize / sizeof(glm::mat4);
		}
		else if (renderItem.materialDataSize > 0)
		{
			vkCmdPushConstants(cb, graphicsPipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, renderItem.materialDataSize, renderItem.materialData);
		}

		instanceDataOffset += renderItem.meshParamsSize / sizeof(glm::mat4);

		if (ib)
		{
			vkCmdBindIndexBuffer(cb, ib->GetBuffer(), 0, VK_INDEX_TYPE_UINT16);

			if (renderItem.mesh->instanceCount > 0)
				vkCmdDrawIndexed(cb, renderItem.mesh->indexCount, renderItem.mesh->instanceCount, 0, 0, renderItem.mesh->instanceOffset);
			else
				vkCmdDrawIndexed(cb, renderItem.mesh->indexCount, 1, 0, 0, 0);
		}
		else
		{
			vkCmdDraw(cb, renderItem.mesh->vertexCount, 1, 0, 0);
		}

		curPipeline = pipeline;
	}

	void VKRenderer::SubmitIndirect(const RenderItem &renderItem, Buffer *indirectBuffer)
	{
		const std::vector<Buffer*> &vbs = renderItem.mesh->vao->GetVertexBuffers();
		VKBuffer* ib = static_cast<VKBuffer*>(renderItem.mesh->vao->GetIndexBuffer());

		std::vector<VkBuffer> vertexBuffers(vbs.size());
		std::vector<VkDeviceSize> vbOffsets(vbs.size());
		for (size_t i = 0; i < vertexBuffers.size(); i++)
		{
			vertexBuffers[i] = static_cast<VKBuffer*>(vbs[i])->GetBuffer();
			vbOffsets[i] = 0;
		}

		const ShaderPass &pass = renderItem.matInstance->baseMaterial->GetShaderPass(renderItem.shaderPass);
		VkPipeline pipeline = pipelines[pass.pipelineID];
		const VkCommandBuffer &cb = frameResources[currentFrame].frameCmdBuffer;

		vkCmdBindPipeline(cb, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);		// TODO: Sort pipelines
		vkCmdBindVertexBuffers(cb, 0, vertexBuffers.size(), vertexBuffers.data(), vbOffsets.data());

		// Check the set to see if it's equal to the previous so we don't bind it
		if (renderItem.matInstance->textures.size() > 0)
			vkCmdBindDescriptorSets(cb, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipelineLayout, 2, 1, &sets[renderItem.matInstance->graphicsSetID].set[currentFrame], 0, nullptr);

		if (renderItem.materialDataSize > 0)
		{
			vkCmdPushConstants(cb, graphicsPipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, renderItem.materialDataSize, renderItem.materialData);
		}

		//VKDrawIndirectBuffer *indBuffer = static_cast<VKDrawIndirectBuffer*>(indirectBuffer);
		VKBuffer* indBuffer = static_cast<VKBuffer*>(indirectBuffer);

		if (ib)
		{
			vkCmdBindIndexBuffer(cb, ib->GetBuffer(), 0, VK_INDEX_TYPE_UINT16);
			vkCmdDrawIndexedIndirect(cb, indBuffer->GetBuffer(), 0, 1, sizeof(VkDrawIndexedIndirectCommand));
		}
		else
		{
			vkCmdDrawIndirect(cb, indBuffer->GetBuffer(), 0, 1, sizeof(VkDrawIndexedIndirectCommand));
		}

		curPipeline = pipeline;
	}

	void VKRenderer::Dispatch(const DispatchItem &item)
	{
		const ShaderPass &pass = item.matInstance->baseMaterial->GetShaderPass(item.shaderPass);

		//VkCommandBuffer cmdBuffer = frameResources[currentFrame].computeCmdBuffer;
		VkCommandBuffer cmdBuffer = frameResources[currentFrame].frameCmdBuffer;
		vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipelines[pass.pipelineID]);
		
		VkPipelineLayout computeLayout = computePipelineLayouts[item.matInstance->computePipelineLayoutIdx];

		// Only bind once on compute when the camera changes
		// On setcamera()  changed=true     and here   if(changed) change set
		uint32_t dynamicOffset = static_cast<uint32_t>(currentCamera) * singleCameraAlignedSize;

		VkDescriptorSet s[] = { frameResources[currentFrame].globalBuffersSet, globalTexturesSet };
		vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, computeLayout, 0, 2, s, 1, &dynamicOffset);

		if (item.matInstance->computeSetID < sets.size())
			vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, computeLayout, 2, 1, &sets[item.matInstance->computeSetID].set[currentFrame], 0, nullptr);

		if (item.materialDataSize > 0 && item.materialDataSize <= sizeof(glm::vec4))
			vkCmdPushConstants(cmdBuffer, computeLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0, static_cast<uint32_t>(item.materialDataSize), item.materialData);

		vkCmdDispatch(cmdBuffer, item.numGroupsX, item.numGroupsY, item.numGroupsZ);
	}

	void VKRenderer::AddTextureResourceToSlot(unsigned int binding, Texture *texture, bool useStorage, unsigned int stages, TextureInternalFormat viewFormat, bool separateMipViews)
	{
		if (!texture)
			return;

		VkShaderStageFlags shaderStages = 0;

		if ((stages & VERTEX) == VERTEX)
			shaderStages |= VK_SHADER_STAGE_VERTEX_BIT;
		if ((stages & GEOMETRY) == GEOMETRY)
			shaderStages |= VK_SHADER_STAGE_GEOMETRY_BIT;
		if ((stages & FRAGMENT) == FRAGMENT)
			shaderStages |= VK_SHADER_STAGE_FRAGMENT_BIT;
		if ((stages & COMPUTE) == COMPUTE)
			shaderStages |= VK_SHADER_STAGE_COMPUTE_BIT;

		if (texture->GetType() == TextureType::TEXTURE2D)
		{
			VKTexture2D *tex = static_cast<VKTexture2D*>(texture);

			VkImageLayout layout;
			VkDescriptorType type;
			const TextureParams &p = tex->GetTextureParams();

			if (p.usedAsStorageInCompute || p.usedAsStorageInGraphics)
			{
				layout = VK_IMAGE_LAYOUT_GENERAL;
				type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
			}
			else if (tex->IsDepthTexture())
			{
				layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
				type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			}
			else
			{
				layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
				type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			}

			VkDescriptorSetLayoutBinding b = {};
			b.binding = binding;
			b.descriptorCount = 1;
			b.descriptorType = type;
			b.stageFlags = shaderStages;

			VkWriteDescriptorSet write = {};
			write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			write.descriptorCount = 1;
			write.descriptorType = type;
			write.dstArrayElement = 0;
			write.dstBinding = binding;

			globalTexturesSetLayoutBindings.push_back(b);
			globalTexturesSetWrites.push_back(write);

			VKImageInfo info = {};
			info.layout = layout;
			info.imageViews.push_back(tex->GetImageView());
			info.index = static_cast<unsigned int>(globalTexturesSetWrites.size() - 1);

			if (useStorage)
				info.sampler = VK_NULL_HANDLE;
			else
				info.sampler = tex->GetSampler();

			imagesInfo.push_back(info);
		}
		else if (texture->GetType() == TextureType::TEXTURE3D)
		{
			VKTexture3D *tex = static_cast<VKTexture3D*>(texture);

			VkImageLayout layout;
			VkDescriptorType type;
			const TextureParams &p = tex->GetTextureParams();

			if ((p.usedAsStorageInCompute || p.usedAsStorageInGraphics) && useStorage)
			{
				layout = VK_IMAGE_LAYOUT_GENERAL;
				type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
			}
			else
			{
				//layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
				layout = VK_IMAGE_LAYOUT_GENERAL;		// We don't yet perform the transition to shader read
				type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			}

			VkDescriptorSetLayoutBinding b = {};
			b.binding = binding;		
			b.descriptorType = type;
			b.stageFlags = shaderStages;

			uint32_t mips = tex->GetMipLevels();

			VKImageInfo info = {};
			info.layout = layout;
			info.separateMipViews = separateMipViews;
			info.mips = mips;

			if (separateMipViews)
			{
				b.descriptorCount = mips;
				for (uint32_t i = 0; i < mips; i++)
				{
					info.imageViews.push_back(tex->GetImageViewForMip(i));
				}
			}
			else
			{
				if (viewFormat != tex->GetTextureParams().internalFormat)
					info.imageViews.push_back(tex->GetImageViewForAllMips(viewFormat));
				else
					info.imageViews.push_back(tex->GetImageViewForAllMips());

				b.descriptorCount = 1;			
			}

			if (useStorage)
				info.sampler = VK_NULL_HANDLE;
			else
				info.sampler = tex->GetSampler();

			
			VkWriteDescriptorSet write = {};
			write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			write.descriptorType = type;
			write.dstArrayElement = 0;
			write.dstBinding = binding;

			if (separateMipViews)
				write.descriptorCount = mips;
			else
				write.descriptorCount = 1;

			globalTexturesSetLayoutBindings.push_back(b);
			globalTexturesSetWrites.push_back(write);

			info.index = static_cast<unsigned int>(globalTexturesSetWrites.size() - 1);

			imagesInfo.push_back(info);
		}
	}

	void VKRenderer::AddBufferResourceToSlot(unsigned int binding, Buffer *buffer, unsigned int stages)
	{
		if (!buffer)
			return;

		VkShaderStageFlags shaderStages = 0;

		if ((stages & VERTEX) == VERTEX)
			shaderStages |= VK_SHADER_STAGE_VERTEX_BIT;
		if ((stages & GEOMETRY) == GEOMETRY)
			shaderStages |= VK_SHADER_STAGE_GEOMETRY_BIT;
		if ((stages & FRAGMENT) == FRAGMENT)
			shaderStages |= VK_SHADER_STAGE_FRAGMENT_BIT;
		if ((stages & COMPUTE) == COMPUTE)
			shaderStages |= VK_SHADER_STAGE_COMPUTE_BIT;

		if (buffer->GetType() == BufferType::UniformBuffer)
		{
			VKBuffer *ubo = static_cast<VKBuffer*>(buffer);

			VkDescriptorSetLayoutBinding b = {};
			b.binding = binding;
			b.descriptorCount = 1;
			b.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			b.stageFlags = shaderStages;

			VkWriteDescriptorSet write = {};
			write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			write.descriptorCount = 1;
			write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			write.dstArrayElement = 0;
			write.dstBinding = binding;

			globalBuffersSetLayoutBindings.push_back(b);
			globalBuffersSetWrites.push_back(write);			

			VKBufferInfo info = {};
			info.buffer = ubo;
			info.binding = static_cast<unsigned int>(globalBuffersSetWrites.size() - 1);

			bufferInfos.push_back(info);
		}
		else if (buffer->GetType() == BufferType::ShaderStorageBuffer)
		{
			VKBuffer* ssbo = static_cast<VKBuffer*>(buffer);

			VkDescriptorSetLayoutBinding b = {};
			b.binding = binding;
			b.descriptorCount = 1;
			b.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
			b.stageFlags = shaderStages;

			VkWriteDescriptorSet write = {};
			write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			write.descriptorCount = 1;
			write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
			write.dstArrayElement = 0;
			write.dstBinding = binding;

			globalBuffersSetLayoutBindings.push_back(b);
			globalBuffersSetWrites.push_back(write);

			VKBufferInfo info = {};
			info.buffer = ssbo;
			info.binding = static_cast<unsigned int>(globalBuffersSetWrites.size() - 1);

			bufferInfos.push_back(info);
		}
		else if (buffer->GetType()==BufferType::DrawIndirectBuffer)
		{
			VKBuffer*indBuffer = static_cast<VKBuffer*>(buffer);

			VkDescriptorSetLayoutBinding b = {};
			b.binding = binding;
			b.descriptorCount = 1;
			b.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
			b.stageFlags = shaderStages;

			VkWriteDescriptorSet write = {};
			write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			write.descriptorCount = 1;
			write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
			write.dstArrayElement = 0;
			write.dstBinding = binding;

			globalBuffersSetLayoutBindings.push_back(b);
			globalBuffersSetWrites.push_back(write);

			VKBufferInfo info = {};
			info.buffer = indBuffer;
			info.binding = static_cast<unsigned int>(globalBuffersSetWrites.size() - 1);

			bufferInfos.push_back(info);
		}
	}

	void VKRenderer::SetupResources()
	{
		VkDescriptorSetLayoutCreateInfo buffersSetLayoutInfo = {};
		buffersSetLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		buffersSetLayoutInfo.bindingCount = static_cast<uint32_t>(globalBuffersSetLayoutBindings.size());
		buffersSetLayoutInfo.pBindings = globalBuffersSetLayoutBindings.data();

		if (vkCreateDescriptorSetLayout(base.GetDevice(), &buffersSetLayoutInfo, nullptr, &globalBuffersSetLayout) != VK_SUCCESS)
		{
			Log::Print(LogLevel::LEVEL_ERROR, "Failed to create buffers descriptor set layout!\n");
			return;
		}

		VkDescriptorSetLayoutCreateInfo texturesSetLayoutInfo = {};
		texturesSetLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		texturesSetLayoutInfo.bindingCount = static_cast<uint32_t>(globalTexturesSetLayoutBindings.size());
		texturesSetLayoutInfo.pBindings = globalTexturesSetLayoutBindings.data();

		if (vkCreateDescriptorSetLayout(base.GetDevice(), &texturesSetLayoutInfo, nullptr, &globalTexturesSetLayout) != VK_SUCCESS)
		{
			Log::Print(LogLevel::LEVEL_ERROR, "Failed to create textures descriptor set layout!\n");
			return;
		}


		// User textures
		VkDescriptorSetLayoutBinding setBindings[8];
		for (size_t i = 0; i < 8; i++)
		{
			VkDescriptorSetLayoutBinding texBinding = {};
			texBinding.binding = static_cast<uint32_t>(i);
			texBinding.descriptorCount = 1;
			texBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			texBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

			setBindings[i] = texBinding;
		}

		VkDescriptorSetLayoutCreateInfo setLayoutInfo = {};
		setLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		setLayoutInfo.bindingCount = 8;
		setLayoutInfo.pBindings = setBindings;

		if (vkCreateDescriptorSetLayout(base.GetDevice(), &setLayoutInfo, nullptr, &userSetLayout) != VK_SUCCESS)
		{
			std::cout << "Failed to create tex descriptor set layout\n";
			return;
		}

		VkDescriptorSetLayout graphicsSetLayouts[] = { globalBuffersSetLayout, globalTexturesSetLayout, userSetLayout };

		VkPushConstantRange pushConstantRange = {};
		pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
		pushConstantRange.offset = 0;
		pushConstantRange.size = 64;

		// Pipeline layout
		VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
		pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutInfo.pushConstantRangeCount = 1;
		pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;
		pipelineLayoutInfo.setLayoutCount = 3;
		pipelineLayoutInfo.pSetLayouts = graphicsSetLayouts;

		if (vkCreatePipelineLayout(base.GetDevice(), &pipelineLayoutInfo, nullptr, &graphicsPipelineLayout) != VK_SUCCESS)
			std::cout << "Failed to create pipeline layout\n";

		VkDescriptorSetAllocateInfo setAllocInfo = {};
		setAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		setAllocInfo.descriptorPool = descriptorPool;
		setAllocInfo.descriptorSetCount = 1;
		setAllocInfo.pSetLayouts = &globalBuffersSetLayout;

		for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
		{
			if (vkAllocateDescriptorSets(base.GetDevice(), &setAllocInfo, &frameResources[i].globalBuffersSet) != VK_SUCCESS)
			{
				Log::Print(LogLevel::LEVEL_ERROR, "Failed to allocate buffers descriptor set\n");
				return;
			}
		}

		setAllocInfo.pSetLayouts = &globalTexturesSetLayout;

		if (vkAllocateDescriptorSets(base.GetDevice(), &setAllocInfo, &globalTexturesSet) != VK_SUCCESS)
		{
			Log::Print(LogLevel::LEVEL_ERROR, "Failed to allocate textures descriptor set\n");
			return;
		}

		/*for (size_t i = 0; i < globalBuffersSetWrites.size(); i++)
		{
			for (int j = 0; j < MAX_FRAMES_IN_FLIGHT; j++)
			{
				globalBuffersSetWrites[i].dstSet = buffersSet;
			}
			
		}*/
		for (size_t i = 0; i < globalTexturesSetWrites.size(); i++)
		{
			globalTexturesSetWrites[i].dstSet = globalTexturesSet;
		}

		for (size_t i = 0; i < bufferInfos.size(); i++)
		{
			const VKBufferInfo& bi = bufferInfos[i];

			VkDescriptorBufferInfo info = {};
			info.buffer = bi.buffer->GetBuffer();
			info.offset = 0;
			info.range = VK_WHOLE_SIZE;

			for (int j = 0; j < MAX_FRAMES_IN_FLIGHT; j++)
			{
				VkWriteDescriptorSet& write = globalBuffersSetWrites[bi.binding];

				if (write.descriptorType == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER)
				{
					// The range is aligned, we can't actually use the other bytes anyway
					// If we use an actual range of 120, the aligned range will be 256. If we use an offset we would have to start at 256
					// and the part between 120->256 can't be used, so it doesn't matter if the range is 120 or 256
					info.range = VkDeviceSize(bi.buffer->GetSize() / MAX_FRAMES_IN_FLIGHT);
					info.offset = VkDeviceSize(j * utils::Align(static_cast<unsigned int>(bi.buffer->GetSize()) / MAX_FRAMES_IN_FLIGHT, static_cast<unsigned int>(base.GetDeviceLimits().minUniformBufferOffsetAlignment)));
				}			
				if (bi.binding == CAMERA_UBO)
				{
					info.offset = VkDeviceSize(j * allCamerasAlignedSize);
				}
				else if (bi.binding == INSTANCE_DATA_SSBO)
				{
					info.range = static_cast<VkDeviceSize>(instanceDataBufferSingleSize);
					info.offset = static_cast<VkDeviceSize>(j * instanceDataBufferSingleSize);
				}

				write.pBufferInfo = &info;
				write.dstSet = frameResources[j].globalBuffersSet;

				vkUpdateDescriptorSets(base.GetDevice(), 1, &globalBuffersSetWrites[bi.binding], 0, nullptr);
			}		
		}

		for (size_t i = 0; i < imagesInfo.size(); i++)
		{
			const VKImageInfo &imgInfo = imagesInfo[i];

			if (imgInfo.separateMipViews)
			{
				std::vector<VkDescriptorImageInfo> infos(imgInfo.mips);
				for (uint32_t j = 0; j < imgInfo.mips; j++)
				{
					infos[j].imageLayout = imgInfo.layout;
					infos[j].sampler = imgInfo.sampler;
					infos[j].imageView = imgInfo.imageViews[j];
				}

				globalTexturesSetWrites[imgInfo.index].pImageInfo = infos.data();

				vkUpdateDescriptorSets(base.GetDevice(), 1, &globalTexturesSetWrites[imgInfo.index], 0, nullptr);
			}
			else
			{
				VkDescriptorImageInfo info = {};
				info.imageLayout = imgInfo.layout;
				info.imageView = imgInfo.imageViews[0];
				info.sampler = imgInfo.sampler;

				globalTexturesSetWrites[imgInfo.index].pImageInfo = &info;

				vkUpdateDescriptorSets(base.GetDevice(), 1, &globalTexturesSetWrites[imgInfo.index], 0, nullptr);
			}		
		}

		//vkUpdateDescriptorSets(base.GetDevice(), static_cast<uint32_t>(globalSetWrites.size()), globalSetWrites.data(), 0, nullptr);

		globalBuffersSetWrites.clear();
		globalTexturesSetWrites.clear();
		globalBuffersSetLayoutBindings.clear();
		globalTexturesSetLayoutBindings.clear();
		bufferInfos.clear();
		imagesInfo.clear();
	}

	void VKRenderer::UpdateTextureResourceOnSlot(unsigned int binding, Texture *texture, bool useStorage, bool separateMipViews)
	{
		VkImageLayout layout;
		VkDescriptorType type;

		const TextureParams &p = texture->GetTextureParams();

		if (p.usedAsStorageInCompute || p.usedAsStorageInGraphics)
		{
			layout = VK_IMAGE_LAYOUT_GENERAL;
			type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
		}
		else if (Texture::IsDepthTexture(p.internalFormat))
		{
			layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
			type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		}
		else
		{
			layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		}

		uint32_t descriptorCount = 1;

		std::vector<VkDescriptorImageInfo> infos;

		if (texture->GetType() == TextureType::TEXTURE2D)
		{
			VKTexture2D *tex = static_cast<VKTexture2D*>(texture);

			infos.resize(1);
			infos[0].imageLayout = layout;
			infos[0].imageView = tex->GetImageView();
			infos[0].sampler = useStorage ? VK_NULL_HANDLE : tex->GetSampler();
		}
		else if (texture->GetType() == TextureType::TEXTURE3D)
		{
			VKTexture3D *tex = static_cast<VKTexture3D*>(texture);

			uint32_t mips = tex->GetMipLevels();

			if (separateMipViews)
			{
				infos.resize(mips);
				descriptorCount = mips;

				for (uint32_t i = 0; i < mips; i++)
				{
					infos[i].imageLayout = layout;
					infos[i].imageView = tex->GetImageViewForMip(i);
					infos[i].sampler = useStorage ? VK_NULL_HANDLE : tex->GetSampler();
				}
			}
			else
			{

				infos.resize(1);
				infos[0].imageLayout = layout;
				infos[0].imageView = tex->GetImageViewForAllMips();
				infos[0].sampler = useStorage ? VK_NULL_HANDLE : tex->GetSampler();
			}
		}

		VkWriteDescriptorSet write = {};
		write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		write.descriptorCount = descriptorCount;
		write.descriptorType = type;
		write.dstArrayElement = 0;
		write.dstBinding = binding;
		write.dstSet = globalTexturesSet;
		write.pImageInfo = infos.data();

		vkUpdateDescriptorSets(base.GetDevice(), 1, &write, 0, nullptr);
	}

	void VKRenderer::PerformBarrier(const Barrier &barrier)
	{
		std::vector<VkImageMemoryBarrier> imbs(barrier.images.size());
		std::vector<VkBufferMemoryBarrier> bmbs(barrier.buffers.size());

		for (size_t i = 0; i < imbs.size(); i++)
		{
			const BarrierImage &bi = barrier.images[i];

			VkImageMemoryBarrier imb = {};
			imb.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
			imb.subresourceRange.baseArrayLayer = 0;
			imb.subresourceRange.layerCount = 1;
			imb.subresourceRange.baseMipLevel = static_cast<uint32_t>(bi.baseMip);
			imb.subresourceRange.levelCount = static_cast<uint32_t>(bi.numMips);

			/*if (bi.transitionToGeneral)
			{
				imb.oldLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
				imb.newLayout = VK_IMAGE_LAYOUT_GENERAL;
			}
			else if (bi.transitionToShaderRead)
			{
				imb.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
				imb.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			}
			else
			{*/
				imb.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
				imb.newLayout = VK_IMAGE_LAYOUT_GENERAL;
			//}
				
			if (bi.readToWrite)
			{
				imb.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
				imb.dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
			}
			else
			{
				imb.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
				imb.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
			}

			Texture *t = bi.image;

			if (t->GetType() == TextureType::TEXTURE2D)
			{
				VKTexture2D *tex = static_cast<VKTexture2D*>(t);

				imb.image = tex->GetImage();
				imb.subresourceRange.aspectMask = tex->GetAspectFlags();

				/*if (tex->IsDepthTexture())
				{
					//imb.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;

					if (bi.readToWrite)
					{
						imb.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
						imb.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;				
						imb.oldLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
						imb.newLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
						//imb.newLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
					}
					else
					{
						imb.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
						imb.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
						//imb.oldLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
						imb.oldLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
						imb.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
					}					
				}*/

			}
			else
			{
				VKTexture3D *tex = static_cast<VKTexture3D*>(t);

				imb.image = tex->GetImage();
				imb.subresourceRange.aspectMask = tex->GetAspectFlags();
			}
			imbs[i] = imb;
		}

		for (size_t i = 0; i < bmbs.size(); i++)
		{
			VkBufferMemoryBarrier bmb = {};
			bmb.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
			
			Buffer *b = barrier.buffers[i].buffer;

			if (b->GetType() == BufferType::DrawIndirectBuffer)
			{
				VKBuffer* buf = static_cast<VKBuffer*>(b);

				if (barrier.buffers[i].readToWrite)
				{
					bmb.srcAccessMask = VK_ACCESS_INDIRECT_COMMAND_READ_BIT;
					bmb.dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
				}
				else
				{
					bmb.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
					bmb.dstAccessMask = VK_ACCESS_INDIRECT_COMMAND_READ_BIT;
				}
				
				bmb.buffer = buf->GetBuffer();
				bmb.offset = 0;
				bmb.size = VK_WHOLE_SIZE;
			}
			else if (b->GetType() == BufferType::ShaderStorageBuffer)
			{
				VKBuffer* buf = static_cast<VKBuffer*>(b);

				if (barrier.buffers[i].readToWrite)
				{
					bmb.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
					bmb.dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
				}
				else
				{
					bmb.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
					bmb.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
				}

				bmb.buffer = buf->GetBuffer();
				bmb.offset = 0;
				bmb.size = VK_WHOLE_SIZE;
			}

			bmbs[i] = bmb;
		}
				
		VkPipelineStageFlags srcStage = 0;
		VkPipelineStageFlags dstStage = 0;

		if (barrier.srcStage & INDIRECT)
			srcStage |= VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT;
		if (barrier.srcStage & VERTEX)
			srcStage |= VK_PIPELINE_STAGE_VERTEX_SHADER_BIT;
		if (barrier.srcStage & GEOMETRY)
			srcStage |= VK_PIPELINE_STAGE_GEOMETRY_SHADER_BIT;
		if (barrier.srcStage & FRAGMENT)
			srcStage |= VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		if (barrier.srcStage & COMPUTE)
			srcStage |= VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
		if (barrier.srcStage & DEPTH_STENCIL_WRITE)
			srcStage |= VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;

		if (barrier.dstStage & INDIRECT)
			dstStage |= VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT;
		if (barrier.dstStage & VERTEX)
			dstStage |= VK_PIPELINE_STAGE_VERTEX_SHADER_BIT;
		if (barrier.dstStage & GEOMETRY)
			dstStage |= VK_PIPELINE_STAGE_GEOMETRY_SHADER_BIT;
		if (barrier.dstStage & FRAGMENT)
			dstStage |= VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		if (barrier.dstStage & COMPUTE)
			dstStage |= VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
		if (barrier.dstStage & DEPTH_STENCIL_WRITE)
			dstStage |= VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;

		
		vkCmdPipelineBarrier(frameResources[currentFrame].frameCmdBuffer, srcStage, dstStage,
			0,
			0, nullptr,
			static_cast<uint32_t>(bmbs.size()), bmbs.data(),
			static_cast<uint32_t>(imbs.size()), imbs.data());
	}

	void VKRenderer::CopyImage(Texture *src, Texture *dst)
	{
		VKTexture2D *srcTex = static_cast<VKTexture2D*>(src);
		VKTexture2D *dstTex = static_cast<VKTexture2D*>(dst);
		
		VkImageCopy region = {};
		region.srcOffset = { 0,0,0 };
		region.srcSubresource.aspectMask = srcTex->GetAspectFlags();
		region.srcSubresource.baseArrayLayer = 0;
		region.srcSubresource.layerCount = 1;
		region.srcSubresource.mipLevel = 0;

		region.dstOffset = { 0,0,0 };
		region.dstSubresource.aspectMask = dstTex->GetAspectFlags();
		region.dstSubresource.baseArrayLayer = 0;
		region.dstSubresource.layerCount = 1;
		region.dstSubresource.mipLevel = 0;

		region.extent.width = srcTex->GetWidth();
		region.extent.height = srcTex->GetHeight();
		region.extent.depth = 1;

		VkOffset3D blitSize;
		blitSize.x = srcTex->GetWidth();
		blitSize.y = srcTex->GetHeight();
		blitSize.z = 1;

		VkImageBlit blit = {};
		blit.srcOffsets[1] = blitSize;
		blit.dstOffsets[1] = blitSize;
		blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		blit.srcSubresource.baseArrayLayer = 0;
		blit.srcSubresource.layerCount = 1;
		blit.srcSubresource.mipLevel = 0;
		blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		blit.dstSubresource.baseArrayLayer = 0;
		blit.dstSubresource.layerCount = 1;
		blit.dstSubresource.mipLevel = 0;

		VkCommandBuffer cmdBuffer = frameResources[currentFrame].frameCmdBuffer;

		// Transition the srcTex from SHADER_READ to TRANSFER_SRC
		// Transition the dstTex to TRANSFER_DST
		VkImageMemoryBarrier barriers[2] = {};
		barriers[0].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		barriers[0].oldLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		barriers[0].newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		barriers[0].srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
		barriers[0].dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
		barriers[0].image = srcTex->GetImage();
		barriers[0].subresourceRange.aspectMask = srcTex->GetAspectFlags();
		barriers[0].subresourceRange.levelCount = 1;
		barriers[0].subresourceRange.layerCount = 1;

		barriers[1].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		barriers[1].oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		barriers[1].newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		barriers[1].srcAccessMask = 0;
		barriers[1].dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barriers[1].image = dstTex->GetImage();
		barriers[1].subresourceRange.aspectMask = dstTex->GetAspectFlags();
		barriers[1].subresourceRange.levelCount = 1;
		barriers[1].subresourceRange.layerCount = 1;

		vkCmdPipelineBarrier(cmdBuffer, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 2, barriers);

		//vkCmdCopyImage(frameResources[currentFrame].frameCmdBuffer, srcTex->GetImage(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, dstTex->GetImage(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

		vkCmdBlitImage(frameResources[currentFrame].frameCmdBuffer, srcTex->GetImage(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, dstTex->GetImage(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &blit, VK_FILTER_NEAREST);

		// Transition both images to SHADER_READ
		barriers[0].oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		barriers[0].newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		barriers[0].srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
		barriers[0].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		barriers[1].oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		barriers[1].newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		barriers[1].srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barriers[1].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		vkCmdPipelineBarrier(cmdBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 2, barriers);
	}

	void VKRenderer::ClearImage(Texture *tex)
	{
		if (tex->GetType() == TextureType::TEXTURE3D)
		{
			VKTexture3D *tex3d = static_cast<VKTexture3D*>(tex);

			VkImageSubresourceRange range = {};
			range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			range.baseArrayLayer = 0;
			range.baseMipLevel = 0;
			range.layerCount = 1;
			range.levelCount = tex->GetMipLevels();

			VkClearColorValue clearColor = {};
			clearColor.float32[0] = 0.0f;
			clearColor.float32[1] = 0.0f;
			clearColor.float32[2] = 0.0f;
			clearColor.float32[3] = 0.0f;

			VkImageMemoryBarrier barrier = {};
			barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
			barrier.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
			barrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
			barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
			barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
			barrier.image = tex3d->GetImage();
			barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			barrier.subresourceRange.levelCount = tex3d->GetMipLevels();
			barrier.subresourceRange.layerCount = 1;

			vkCmdPipelineBarrier(frameResources[currentFrame].frameCmdBuffer, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);

			// TODO: Warning because of GENERAL layout, transition to TRANSFER_DST_OPTIMAL ? test
			vkCmdClearColorImage(frameResources[currentFrame].frameCmdBuffer, tex3d->GetImage(), VK_IMAGE_LAYOUT_GENERAL, &clearColor, 1, &range);

			barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
			barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;

			vkCmdPipelineBarrier(frameResources[currentFrame].frameCmdBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);
		}
		
	}

	void VKRenderer::BeginFrame()
	{
		currentCamera = 0;

		VkDevice device = base.GetDevice();

		if (needsTransfers)
		{
			vkEndCommandBuffer(transferCommandBuffer);

			VkSubmitInfo submitInfo = {};
			submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
			submitInfo.commandBufferCount = 1;
			submitInfo.pCommandBuffers = &transferCommandBuffer;

			if (vkQueueSubmit(base.GetTransferQueue(), 1, &submitInfo, VK_NULL_HANDLE) != VK_SUCCESS)
			{
				throw std::runtime_error("failed to submit transfer command buffer!");
			}

			vkQueueWaitIdle(base.GetTransferQueue());

			VkCommandBufferBeginInfo beginInfo = {};
			beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
			beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
			vkBeginCommandBuffer(transferCommandBuffer, &beginInfo);

			needsTransfers = false;
			// Dispose the staging buffers
			DisposeStagingResources();
		}

		vkWaitForFences(device, 1, &frameResources[currentFrame].frameFence, VK_TRUE, std::numeric_limits<uint64_t>::max());
		vkResetFences(device, 1, &frameResources[currentFrame].frameFence);

		/*vkWaitForFences(device, 1, &frameResources[currentFrame].computeFence, VK_TRUE, std::numeric_limits<uint64_t>::max());
		vkResetFences(device, 1, &frameResources[currentFrame].computeFence);*/

		// Update this frame descriptor sets before we begin the command buffer
		UpdateDescriptorSets();

		if (framesWaitedToRemove >= MAX_FRAMES_IN_FLIGHT)
		{
			for (size_t i = 0; i < texturesToRemove.size(); i++)
			{
				texturesToRemove[i]->RemoveReference();
			}
			texturesToRemove.clear();
			framesWaitedToRemove = 0;
		}

		VkCommandBufferBeginInfo beginInfo = {};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

		vkBeginCommandBuffer(frameResources[currentFrame].frameCmdBuffer, &beginInfo);
		//vkBeginCommandBuffer(frameResources[currentFrame].computeCmdBuffer, &beginInfo);

		vkCmdBindDescriptorSets(frameResources[currentFrame].frameCmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipelineLayout, 1, 1, &globalTexturesSet, 0, nullptr);

		instanceDataOffset = 0;
		mappedInstanceData = (char*)instanceDataSSBO->Mapped() + currentFrame * instanceDataBufferSingleSize;
	}

	void VKRenderer::Present()
	{
		if (vkEndCommandBuffer(frameResources[currentFrame].frameCmdBuffer) != VK_SUCCESS)
			Log::Print(LogLevel::LEVEL_ERROR, "Failed to record command buffer\n");
		/*if (vkEndCommandBuffer(frameResources[currentFrame].computeCmdBuffer) != VK_SUCCESS)
			Log::Print(LogLevel::LEVEL_ERROR, "Failed to record compute command buffer\n");*/

		VkDevice device = base.GetDevice();

		cameraUBO->Update(cameraUBOData, currentCamera * singleCameraAlignedSize, currentFrame * allCamerasAlignedSize);

		instanceDataSSBO->Flush(currentFrame * instanceDataBufferSingleSize);

		// Acquire the image as late as possible
		VkResult res = vkAcquireNextImageKHR(device, swapChain.GetSwapChain(), std::numeric_limits<uint64_t>::max(), frameResources[currentFrame].imageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex);
		if (wasResized)
		{
			if (currentFrame != imageIndex)
			{
				// Switch the resources
				FrameResources fr = frameResources[currentFrame];
				frameResources[currentFrame].frameCmdBuffer = frameResources[!currentFrame].frameCmdBuffer;
				//frameResources[currentFrame].computeCmdBuffer = frameResources[!currentFrame].computeCmdBuffer;
				frameResources[currentFrame].frameFence = frameResources[!currentFrame].frameFence;
				//frameResources[currentFrame].computeFence = frameResources[!currentFrame].computeFence;
				frameResources[currentFrame].imageAvailableSemaphore = frameResources[!currentFrame].imageAvailableSemaphore;
				frameResources[currentFrame].renderFinishedSemaphore = frameResources[!currentFrame].renderFinishedSemaphore;

				frameResources[!currentFrame].frameCmdBuffer = fr.frameCmdBuffer;
				//frameResources[!currentFrame].frameCmdBuffer = fr.computeCmdBuffer;
				frameResources[!currentFrame].frameFence = fr.frameFence;
				//frameResources[!currentFrame].computeFence = fr.computeFence;
				frameResources[!currentFrame].imageAvailableSemaphore = fr.imageAvailableSemaphore;
				frameResources[!currentFrame].renderFinishedSemaphore = fr.renderFinishedSemaphore;

				currentFrame = !currentFrame;
			}
			wasResized = false;
			//return;
		}
		// TODO:Recreate swapchain
		if (res == VK_ERROR_OUT_OF_DATE_KHR || res == VK_SUBOPTIMAL_KHR)
			std::cout << "out of date\n";

		assert(imageIndex == currentFrame);

		// Submit compute
		/*VkQueue computeQueue = base.GetComputeQueue();

		VkSubmitInfo computeSubmitInfo = {};
		computeSubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		//submitInfo.pWaitDstStageMask = waitStages;
		//submitInfo.waitSemaphoreCount = 1;
		//submitInfo.signalSemaphoreCount = 1;
		computeSubmitInfo.commandBufferCount = 1;
		computeSubmitInfo.pCommandBuffers = &frameResources[currentFrame].computeCmdBuffer;
		//submitInfo.pWaitSemaphores = &frameResources[currentFrame].imageAvailableSemaphore;				// Wait for present to finish
		//submitInfo.pSignalSemaphores = &frameResources[currentFrame].renderFinishedSemaphore;

		if (vkQueueSubmit(computeQueue, 1, &computeSubmitInfo, frameResources[currentFrame].computeFence) != VK_SUCCESS)
			throw std::runtime_error("Failed to submit!\n");
		*/

		// Submit graphics

		VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };

		VkQueue graphicsQueue = base.GetGraphicsQueue();
		
		VkSubmitInfo submitInfo = {};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.pWaitDstStageMask = waitStages;
		submitInfo.waitSemaphoreCount = 1;
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &frameResources[currentFrame].frameCmdBuffer;
		submitInfo.pWaitSemaphores = &frameResources[currentFrame].imageAvailableSemaphore;
		submitInfo.pSignalSemaphores = &frameResources[currentFrame].renderFinishedSemaphore;
		
		if (vkQueueSubmit(graphicsQueue, 1, &submitInfo, frameResources[currentFrame].frameFence) != VK_SUCCESS)
		{
			Log::Print(LogLevel::LEVEL_ERROR, "Failed to submit!\n");
			return;
		}

		
		// Present

		VkSwapchainKHR swapchain = swapChain.GetSwapChain();

		VkPresentInfoKHR presentInfo = {};
		presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		presentInfo.waitSemaphoreCount = 1;
		presentInfo.pWaitSemaphores = &frameResources[currentFrame].renderFinishedSemaphore;
		presentInfo.swapchainCount = 1;
		presentInfo.pSwapchains = &swapchain;
		presentInfo.pImageIndices = &imageIndex;
		presentInfo.pResults = nullptr;

		res = vkQueuePresentKHR(base.GetPresentQueue(), &presentInfo);
		if (res == VK_ERROR_OUT_OF_DATE_KHR || res == VK_SUBOPTIMAL_KHR)
			std::cout << "out of date\n";
		
		//vkQueueWaitIdle(base.GetPresentQueue());
		//WaitIdle();
		currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;

		if (texturesToRemove.size() > 0)
			framesWaitedToRemove++;
	}

	void VKRenderer::WaitIdle()
	{
		vkDeviceWaitIdle(base.GetDevice());
	}

	void VKRenderer::UpdateMaterialInstance(MaterialInstance *matInst)
	{
		if (!matInst || matInst->textures.size() == 0)
			return;

		bool found = false;

		for (size_t i = 0; i < materialInstances.size(); i++)
		{
			if (materialInstances[i] == matInst)
			{
				found = true;
				break;
			}
		}

		if (found)
		{
			DescriptorsInfo *setInfo = nullptr;

			if (matInst->graphicsSetID == std::numeric_limits<unsigned int>::max() && matInst->computeSetID != std::numeric_limits<unsigned int>::max())
				setInfo = &sets[matInst->computeSetID];
			else
				setInfo = &sets[matInst->graphicsSetID];
			

			DescriptorsUpdateInfo info = {};
			info.matInst = matInst;

			if (setInfo->descUpdateIndex[0] < setsToUpdate[0].size())
			{
				// Only add a new entry to the sets to update if the material instance is not the same, otherwise we would be duplicating it
				if (setsToUpdate[0][setInfo->descUpdateIndex[0]].matInst != matInst)
				{
					info.set = setInfo->set[0];
					setsToUpdate[0].push_back(info);
					setInfo->descUpdateIndex[0] = static_cast<unsigned int>(setsToUpdate[0].size() - 1);
				}
			}
			else
			{
				info.set = setInfo->set[0];
				setsToUpdate[0].push_back(info);
				setInfo->descUpdateIndex[0] = static_cast<unsigned int>(setsToUpdate[0].size() - 1);
			}

			if (setInfo->descUpdateIndex[1] < setsToUpdate[1].size())
			{
				// Only add a new entry to the sets to update if the material instance is not the same, otherwise we would be duplicating it
				if (setsToUpdate[1][setInfo->descUpdateIndex[1]].matInst != matInst)
				{
					info.set = setInfo->set[1];
					setsToUpdate[1].push_back(info);
					setInfo->descUpdateIndex[1] = static_cast<unsigned int>(setsToUpdate[1].size() - 1);
				}
			}
			else
			{
				info.set = setInfo->set[1];
				setsToUpdate[1].push_back(info);
				setInfo->descUpdateIndex[1] = static_cast<unsigned int>(setsToUpdate[1].size() - 1);
			}		
		}
	}

	void VKRenderer::ReloadShaders()
	{
		// We can use this because reloading shaders will only happen when focusing on the editor
		vkDeviceWaitIdle(base.GetDevice());

		Log::Print(LogLevel::LEVEL_INFO, "Reloading shaders...\n");

		struct MatAndShaderPass
		{
			MaterialInstance* mi;
			ShaderPass sp;
		};

		std::vector<MatAndShaderPass> passesToReload;

		const std::map<unsigned int, MaterialRefInfo>& materials = ResourcesLoader::GetMaterials();

		for (auto it = materials.begin(); it != materials.end(); it++)
		{
			std::vector<ShaderPass>& passes = it->second.mat->GetShaderPasses();

			for (size_t i = 0; i < passes.size(); i++)
			{
				ShaderPass& sp = passes[i];

				// Compute passes are handled below
				if (sp.isCompute)
					continue;

				if (sp.shader->CheckIfModified())
				{
					MatAndShaderPass msp = {};
					msp.mi = nullptr;
					msp.sp = sp;
					passesToReload.push_back(msp);
				}
			}
		}

		// We need to also loop through the material instances because of compute materials, which need the material instance passed in when creating the pipeline
		for (size_t i = 0; i < materialInstances.size(); i++)
		{
			MaterialInstance* mi = materialInstances[i];
			std::vector<ShaderPass>& passes = mi->baseMaterial->GetShaderPasses();

			for (size_t i = 0; i < passes.size(); i++)
			{
				ShaderPass& sp = passes[i];

				if (!sp.isCompute)
					continue;

				if (sp.shader->CheckIfModified())
				{
					MatAndShaderPass msp = {};
					msp.mi = mi;
					msp.sp = sp;
					passesToReload.push_back(msp);
				}
			}
		}

		for (size_t i = 0; i < passesToReload.size(); i++)
		{
			MatAndShaderPass& msp = passesToReload[i];

			if (msp.sp.shader->IsCompiled() == false)
				msp.sp.shader->Reload();

			if (!CreatePipeline(msp.sp, msp.mi, true))
			{
				Log::Print(LogLevel::LEVEL_ERROR, "Failed to recreate pipeline!");
			}
		}

		Log::Print(LogLevel::LEVEL_INFO, "Done!\n");
	}

	void VKRenderer::RemoveTexture(Texture* t)
	{
		for (auto it = textures.begin(); it != textures.end(); it++)
		{
			Texture* tex = it->second;

			if (tex == t)
			{
				if (tex->GetRefCount() > 1)
				{
					Log::Print(LogLevel::LEVEL_WARNING, "Calling Renderer::RemoveTexture on a texture with more than 1 reference\n");
					Log::Print(LogLevel::LEVEL_WARNING, "Texture %s still has %u references\n", tex->GetPath().c_str(), tex->GetRefCount());
				}
				else
				{
					Log::Print(LogLevel::LEVEL_INFO, "Removed texture %s\n", tex->GetPath().c_str());
					texturesToRemove.push_back(tex);
					textures.erase(it);
				}		
				return;
			}
		}
	}

	void VKRenderer::Dispose()
	{
		VkDevice device = base.GetDevice();

		vkDeviceWaitIdle(device);

		for (auto it = shaderPrograms.begin(); it != shaderPrograms.end(); it++)
		{
			delete it->second;
		}
		for (auto it = textures.begin(); it != textures.end(); it++)
		{
			if (it->second->GetRefCount() > 1)
				std::cout << it->second->GetPath() << '\n';

			if (it->second->IsAttachment() == false)
				it->second->RemoveReference();
		}
		for (size_t i = 0; i < materialInstances.size(); i++)
		{
			if (materialInstances[i])
				delete materialInstances[i];
		}
		materialInstances.clear();

		for (size_t i = 0; i < vertexBuffers.size(); i++)
		{
			vertexBuffers[i]->RemoveReference();
		}
		for (size_t i = 0; i < indexBuffers.size(); i++)
		{
			indexBuffers[i]->RemoveReference();
		}
		for (size_t i = 0; i < ubos.size(); i++)
		{
			ubos[i]->RemoveReference();
		}
		for (size_t i = 0; i < drawIndirectBufs.size(); i++)
		{
			drawIndirectBufs[i]->RemoveReference();
		}
		for (size_t i = 0; i < ssbos.size(); i++)
		{
			ssbos[i]->RemoveReference();
		}

		vertexBuffers.clear();
		indexBuffers.clear();
		ubos.clear();
		drawIndirectBufs.clear();
		ssbos.clear();

		if (cameraUBO)
			delete cameraUBO;
		if (instanceDataSSBO)
			delete instanceDataSSBO;
		if (cameraUBOData)
			free(cameraUBOData);

		for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
		{
			vkDestroySemaphore(device, frameResources[i].renderFinishedSemaphore, nullptr);
			vkDestroySemaphore(device, frameResources[i].imageAvailableSemaphore, nullptr);
			vkDestroyFence(device, frameResources[i].frameFence, nullptr);
			//vkDestroyFence(device, frameResources[i].computeFence, nullptr);
		}

		for (size_t i = 0; i < pipelines.size(); i++)
		{
			vkDestroyPipeline(device, pipelines[i], nullptr);
		}
		for (size_t i = 0; i < computePipelineLayouts.size(); i++)
		{
			vkDestroyPipelineLayout(device, computePipelineLayouts[i], nullptr);
		}
		for (size_t i = 0; i < computeSecondsSetLayouts.size(); i++)
		{
			vkDestroyDescriptorSetLayout(device, computeSecondsSetLayouts[i], nullptr);
		}

		vkDestroyPipelineLayout(device, graphicsPipelineLayout, nullptr);
		vkDestroyDescriptorSetLayout(device, globalBuffersSetLayout, nullptr);
		vkDestroyDescriptorSetLayout(device, globalTexturesSetLayout, nullptr);
		vkDestroyDescriptorSetLayout(device, userSetLayout, nullptr);
		vkDestroyRenderPass(device, defaultRenderPass, nullptr);

		if (descriptorPool != VK_NULL_HANDLE)
			vkDestroyDescriptorPool(device, descriptorPool, nullptr);

		for (size_t i = 0; i < framebuffers.size(); i++)
		{
			framebuffers[i]->Dispose();
			framebuffers[i]->RemoveReference();
		}

		swapChain.Dispose(device);
		base.Dispose();
		Log::Print(LogLevel::LEVEL_INFO, "Vulkan renderer shutdown\n");
	}

	void VKRenderer::DisposeStagingResources()
	{
		VkPhysicalDevice physicalDevice = base.GetPhysicalDevice();
		VkDevice device = base.GetDevice();

		for (size_t i = 0; i < vertexBuffers.size(); i++)
		{
			VKBuffer* vb = vertexBuffers[i];
			vb->DisposeStagingBuffer();
		}

		for (size_t i = 0; i < indexBuffers.size(); i++)
		{
			VKBuffer* ib = indexBuffers[i];
			ib->DisposeStagingBuffer();
		}

		for (auto it = textures.begin(); it != textures.end(); it++)
		{
			Texture *tex = it->second;

			if (tex->GetType() == TextureType::TEXTURE2D)
			{
				VKTexture2D *tex2D = static_cast<VKTexture2D*>(tex);
				tex2D->DisposeStagingBuffer();
			}
			else if (tex->GetType() == TextureType::TEXTURE3D)
			{
				VKTexture3D *tex3D = static_cast<VKTexture3D*>(tex);
				tex3D->DisposeStagingBuffer();
			}
			else if (tex->GetType() == TextureType::TEXTURE_CUBE)
			{
				VKTextureCube *texCube = static_cast<VKTextureCube*>(tex);
				texCube->DisposeStagingBuffer();
			}
		}
	}

	void VKRenderer::CreateVertexInputState(const std::vector<VertexInputDesc> &descs, std::vector<VkVertexInputBindingDescription> &bindings, std::vector<VkVertexInputAttributeDescription> &attribs)
	{
		bindings.resize(descs.size());

		uint32_t location = 0;

		for (size_t i = 0; i < descs.size(); i++)
		{
			const VertexInputDesc &desc = descs[i];

			bindings[i].binding = i;
			bindings[i].stride = desc.stride;

			if (desc.instanced)
				bindings[i].inputRate = VK_VERTEX_INPUT_RATE_INSTANCE;
			else
				bindings[i].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

			for (size_t j = 0; j < desc.attribs.size(); j++)
			{
				const VertexAttribute &v = desc.attribs[j];

				VkVertexInputAttributeDescription attrib = {};
				attrib.binding = i;
				attrib.location = location;
				attrib.offset = v.offset;

				VkFormat format = VK_FORMAT_UNDEFINED;

				if (v.count == 4 && v.vertexAttribFormat == VertexAttributeFormat::FLOAT)
					attrib.format = VK_FORMAT_R32G32B32A32_SFLOAT;
				else if (v.count == 3 && v.vertexAttribFormat == VertexAttributeFormat::FLOAT)
					attrib.format = VK_FORMAT_R32G32B32_SFLOAT;
				else if (v.count == 2 && v.vertexAttribFormat == VertexAttributeFormat::FLOAT)
					attrib.format = VK_FORMAT_R32G32_SFLOAT;
				else if (v.count == 1 && v.vertexAttribFormat == VertexAttributeFormat::FLOAT)
					attrib.format = VK_FORMAT_R32_SFLOAT;
				else if (v.count == 4 && v.vertexAttribFormat == VertexAttributeFormat::INT)
					attrib.format = VK_FORMAT_R32G32B32A32_SINT;

				attribs.push_back(attrib);

				location++;
			}
		}
	}

	bool VKRenderer::CreatePipeline(ShaderPass &p, MaterialInstance *mat, bool reload)
	{
		VkPipeline pipeline;
		
		if (p.isCompute)
		{
			std::vector<VkDescriptorSetLayoutBinding> setBindings;
			for (size_t i = 0; i < mat->textures.size(); i++)
			{
				VkDescriptorSetLayoutBinding binding = {};
				binding.binding = static_cast<uint32_t>(i);
				binding.descriptorCount = 1;
				binding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

				if (mat->baseMaterial->GetTexturesInfo()[i].params.usedAsStorageInCompute)
					binding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
				else
					binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;			

				setBindings.push_back(binding);
			}

			// Add buffers

			for (size_t i = 0; i < mat->buffers.size(); i++)
			{
				VkDescriptorSetLayoutBinding binding = {};
				binding.binding = static_cast<uint32_t>(setBindings.size());
				binding.descriptorCount = 1;
				binding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
				binding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;

				setBindings.push_back(binding);

			}

			VkDescriptorSetLayoutCreateInfo setLayoutInfo = {};
			setLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
			setLayoutInfo.bindingCount = static_cast<uint32_t>(setBindings.size());
			setLayoutInfo.pBindings = setBindings.data();

			VkDescriptorSetLayout setLayout;

			if (vkCreateDescriptorSetLayout(base.GetDevice(), &setLayoutInfo, nullptr, &setLayout) != VK_SUCCESS)
			{
				std::cout << "Failed to create tex descriptor set layout\n";
				return false;
			}

			computeSecondsSetLayouts.push_back(setLayout);			

			VkDescriptorSetLayout computeSetLayouts[] = { globalBuffersSetLayout, globalTexturesSetLayout, setLayout };

			VkPushConstantRange pushConstantRange = {};
			pushConstantRange.offset = 0;
			pushConstantRange.size = sizeof(glm::vec4);
			pushConstantRange.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

			// Pipeline layout
			VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
			pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
			pipelineLayoutInfo.pushConstantRangeCount = 1;
			pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;

			if (mat->buffers.size() > 0 || mat->textures.size() > 0)
			{
				VkDescriptorSetLayout computeSetLayouts[] = { globalBuffersSetLayout, globalTexturesSetLayout, setLayout };
				pipelineLayoutInfo.setLayoutCount = 3;
				pipelineLayoutInfo.pSetLayouts = computeSetLayouts;
			}
			else
			{
				VkDescriptorSetLayout computeSetLayouts[] = { globalBuffersSetLayout, globalTexturesSetLayout };
				pipelineLayoutInfo.setLayoutCount = 2;
				pipelineLayoutInfo.pSetLayouts = computeSetLayouts;
			}

			VkPipelineLayout pipelineLayout;
			if (vkCreatePipelineLayout(base.GetDevice(), &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS)
			{
				Log::Print(LogLevel::LEVEL_ERROR, "Failed to create compute pipeline layout\n");
				return false;
			}

			computePipelineLayouts.push_back(pipelineLayout);

			// Compile the shader instead of loading the spirv so that we can set flags. Could use a bool to know if we load or compile shader. Eg compile when in editor but don't compile when loading a game
			VKShader *s = static_cast<VKShader*>(p.shader);
			if (!s->Compile(base.GetDevice()))
				return false;

			if (!s->CreateShaderModule(base.GetDevice()))
			{
				Log::Print(LogLevel::LEVEL_ERROR, "Failed to create shader modules!");
				return false;
			}

			VkComputePipelineCreateInfo pipelineInfo = {};
			pipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
			pipelineInfo.flags = 0;
			pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
			pipelineInfo.basePipelineIndex = -1;
			pipelineInfo.layout = pipelineLayout;
			pipelineInfo.stage = s->GetComputeStageInfo();

			if (vkCreateComputePipelines(base.GetDevice(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline) != VK_SUCCESS)
			{
				Log::Print(LogLevel::LEVEL_ERROR, "Failed to create graphics pipeline!\n");
				return false;
			}
			// We don't need the shader modules after pipeline creation
			s->Dispose(base.GetDevice());
		}
		else
		{
			// Find the render pass
			VkRenderPass renderPass = VK_NULL_HANDLE;
			VKFramebuffer *fb = nullptr;
			size_t fbIndex = std::numeric_limits<size_t>::max();
			for (size_t j = 0; j < framebuffers.size(); j++)
			{
				if (p.id == framebuffers[j]->GetPassID())
				{
					renderPass = framebuffers[j]->GetRenderPass();
					fb = framebuffers[j];
					fbIndex = j;
					break;
				}
			}

			if (renderPass == VK_NULL_HANDLE)
			{
				Log::Print(LogLevel::LEVEL_INFO, "Failed to find compatible render pass for material! Using default\n");
				renderPass = defaultRenderPass;
			}

			// Vertex input
			std::vector<VkVertexInputBindingDescription> bindings;
			std::vector<VkVertexInputAttributeDescription> attribs;
			CreateVertexInputState(p.vertexInputDescs, bindings, attribs);

			VkPipelineVertexInputStateCreateInfo vertexInput = vkutils::init::VertexInput(bindings.size(), bindings.data(), attribs.size(), attribs.data());
			// Input assembly
			VkPipelineInputAssemblyStateCreateInfo inputAssembly = vkutils::init::InputAssembly(static_cast<VkPrimitiveTopology>(p.topology), 0);
			// Rasterization
			VkPipelineRasterizationStateCreateInfo rasterization = vkutils::init::Rasterization(VK_POLYGON_MODE_FILL, static_cast<VkCullModeFlags>(p.rasterizerState.cullFace), static_cast<VkFrontFace>(p.rasterizerState.frontFace));

			if (p.topology == 1)
				rasterization.lineWidth = 2.0f;


			// Multisample
			VkPipelineMultisampleStateCreateInfo multisample = vkutils::init::Multisample(VK_SAMPLE_COUNT_1_BIT);
			// Blend attachment
			VkPipelineColorBlendAttachmentState blendAttachment = vkutils::init::BlendAttachment(p.blendState.enableBlending);

			if (!p.blendState.enableColorWriting)
				blendAttachment.colorWriteMask = 0;

			// Blend state
			VkPipelineColorBlendStateCreateInfo blendState = vkutils::init::BlendState(1, &blendAttachment);
			if (fbIndex < framebuffers.size())
			{
				if (framebuffers[fbIndex]->IsDepthOnly())
					blendState = vkutils::init::BlendState(0, nullptr);
			}
			// Depth stencil
			VkPipelineDepthStencilStateCreateInfo depthStencil = vkutils::init::DepthStencil(p.depthStencilState.depthEnable, p.depthStencilState.depthWrite, static_cast<VkCompareOp>(p.depthStencilState.depthFunc));

			VkExtent2D extent;
			if (fb)
			{
				extent.width = static_cast<uint32_t>(fb->GetWidth());
				extent.height = static_cast<uint32_t>(fb->GetHeight());
			}
			else
			{
				extent = swapChain.GetExtent();
			}

			VkViewport viewport = {};
			viewport.x = 0.0f;
			viewport.y = 0.0f;
			viewport.width = (float)extent.width;
			viewport.height = (float)extent.height;
			viewport.minDepth = 0.0f;
			viewport.maxDepth = 1.0f;

			// Scissor
			VkRect2D scissor = {};
			scissor.offset = { 0, 0 };
			scissor.extent = { static_cast<uint32_t>(monitorWidth), static_cast<uint32_t>(monitorHeight) };		// Set the scissor to the monitor resolution. Like this we don't have to worry about dynamic scissor or when resizing
			if (extent.width > monitorWidth)
			{
				scissor.extent.width = extent.width;
			}
			if (extent.height > monitorHeight)
			{
				scissor.extent.height = extent.height;
			}


			VkPipelineViewportStateCreateInfo viewportState = {};
			viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
			viewportState.viewportCount = 1;
			viewportState.pViewports = &viewport;
			viewportState.scissorCount = 1;
			viewportState.pScissors = &scissor;

			// Dynamic state
			VkDynamicState dynamicState[] = { VK_DYNAMIC_STATE_VIEWPORT };

			VkPipelineDynamicStateCreateInfo dynamicInfo = {};
			dynamicInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
			dynamicInfo.pDynamicStates = dynamicState;
			dynamicInfo.dynamicStateCount = 1;
			dynamicInfo.flags = 0;

			VkGraphicsPipelineCreateInfo pipelineInfo = {};
			pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
			pipelineInfo.pInputAssemblyState = &inputAssembly;
			pipelineInfo.pViewportState = &viewportState;
			pipelineInfo.pRasterizationState = &rasterization;
			pipelineInfo.pMultisampleState = &multisample;
			pipelineInfo.pDepthStencilState = &depthStencil;
			pipelineInfo.pColorBlendState = &blendState;
			pipelineInfo.pDynamicState = &dynamicInfo;
			pipelineInfo.layout = graphicsPipelineLayout;
			pipelineInfo.renderPass = renderPass;
			pipelineInfo.subpass = 0;
			pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
			pipelineInfo.basePipelineIndex = -1;

			if (p.vertexInputDescs.size() > 0)
				pipelineInfo.pVertexInputState = &vertexInput;
			else
				pipelineInfo.pVertexInputState = nullptr;


			// Compile the shader instead of loading the spirv so that we can set flags. Could use a bool to know if we load or compile shader. Eg compile when in editor but don't compile when loading a game
			VKShader *s = static_cast<VKShader*>(p.shader);
			if(!s->Compile(base.GetDevice()))
				return false;

			if (!s->CreateShaderModule(base.GetDevice()))
			{
				Log::Print(LogLevel::LEVEL_ERROR, "Failed to create shader modules!");
				return false;
			}

			if (s->HasGeometry())
			{
				VkPipelineShaderStageCreateInfo shaderStages[] = { s->GetVertexStageInfo(), s->GetGeometryStageInfo(), s->GetFragmentStageInfo() };
				pipelineInfo.stageCount = 3;
				pipelineInfo.pStages = shaderStages;

				if (vkCreateGraphicsPipelines(base.GetDevice(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline) != VK_SUCCESS)
				{
					Log::Print(LogLevel::LEVEL_ERROR, "Failed to create graphics pipeline!\n");
					return false;
				}
			}
			else
			{
				VkPipelineShaderStageCreateInfo shaderStages[] = { s->GetVertexStageInfo(), s->GetFragmentStageInfo() };			
				pipelineInfo.stageCount = 2;
				pipelineInfo.pStages = shaderStages;

				if (vkCreateGraphicsPipelines(base.GetDevice(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline) != VK_SUCCESS)
				{
					Log::Print(LogLevel::LEVEL_ERROR, "Failed to create graphics pipeline!\n");
					return false;
				}
			}

			// We don't need the shader modules after pipeline creation
			s->Dispose(base.GetDevice());
		}
		
		if (reload)
		{
			VkPipeline old = pipelines[p.pipelineID];
			pipelines[p.pipelineID] = pipeline;
			vkDestroyPipeline(base.GetDevice(), old, nullptr);
		}
		else
		{
			pipelines.push_back(pipeline);
		}

		return true;
	}

	bool VKRenderer::CreateDefaultRenderPass()
	{
		if (defaultRenderPass != VK_NULL_HANDLE)
			vkDestroyRenderPass(base.GetDevice(), defaultRenderPass, nullptr);

		// Clear the attachment to a color at the start. Rendered contents will be stored in memory and can be read later  // Images to be presented in the swap chain
		VkAttachmentDescription colorAttachment = vkutils::init::AttachmentDesc(swapChain.GetSurfaceFormat(), VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);			// We don't care about the previous layout the image was in. We are going to clear it anyway
		VkAttachmentDescription depthAttachment = vkutils::init::AttachmentDesc(swapChain.GetDepthFormat(), VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_DONT_CARE, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);

		// Sub passes
		VkAttachmentReference colorAttachmentRef = {};
		colorAttachmentRef.attachment = 0;										// Which attachment to reference by its index in the attachment descriptions array. We only have one so its 0
		colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;	// Which layout we would like the attachment to have during a subpass that uses this reference. Vulkan automatically transition the attachment
																				// to this layout when the subpass is started. We intend to use the attachment as a color buffer and VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
																				// layout will give us the best performance.
		VkAttachmentReference depthAttachmentRef = {};
		depthAttachmentRef.attachment = 1;
		depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		VkSubpassDescription subpassDesc = {};
		subpassDesc.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpassDesc.colorAttachmentCount = 1;
		subpassDesc.pColorAttachments = &colorAttachmentRef;
		subpassDesc.pDepthStencilAttachment = &depthAttachmentRef;

		// Subpasses in a render pass automatically take care of image layout transitions. These transitions are controlled by subpass dependencies
		VkSubpassDependency dependency = {};
		dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
		dependency.dstSubpass = 0;
		dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;					// We need to wait for the swap chain to finish reading from the image before we can access it
		dependency.srcAccessMask = 0;
		dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

		std::array<VkAttachmentDescription, 2> attachments = { colorAttachment, depthAttachment };

		VkRenderPassCreateInfo renderPassInfo = {};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
		renderPassInfo.pAttachments = attachments.data();
		renderPassInfo.subpassCount = 1;
		renderPassInfo.pSubpasses = &subpassDesc;
		renderPassInfo.dependencyCount = 1;
		renderPassInfo.pDependencies = &dependency;

		if (vkCreateRenderPass(base.GetDevice(), &renderPassInfo, nullptr, &defaultRenderPass) != VK_SUCCESS)
		{
			Log::Print(LogLevel::LEVEL_ERROR, "Failed to create render pass\n");
			return false;
		}

		return true;
	}

	void VKRenderer::PrepareTexture2D(VKTexture2D *tex)
	{
		// We need to check if the copy regions is more than because we could have created an empty texture
		if (tex->GetCopyRegions().size() > 0)
		{
			if (tex->WantsMipmaps() && tex->HasMipmaps() == false)
			{
				base.TransitionImageLayout(transferCommandBuffer, tex, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1);
				const std::vector<VkBufferImageCopy> &copyRegions = tex->GetCopyRegions();
				vkCmdCopyBufferToImage(transferCommandBuffer, tex->GetStagingBuffer(), tex->GetImage(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, static_cast<uint32_t>(copyRegions.size()), copyRegions.data());
				// To be able to generate mipmaps we need to transition it to transfer_src
				base.TransitionImageLayout(transferCommandBuffer, tex, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, 1);

				for (uint32_t i = 1; i < tex->GetMipLevels(); i++)
				{
					VkImageBlit blit = {};

					// Source
					blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
					blit.srcSubresource.layerCount = 1;
					blit.srcSubresource.mipLevel = i - 1;
					blit.srcOffsets[1].x = static_cast<int32_t>(tex->GetWidth() >> (i - 1));
					blit.srcOffsets[1].y = static_cast<int32_t>(tex->GetHeight() >> (i - 1));
					blit.srcOffsets[1].z = 1;

					// For non-square textures otherwise when getting to the end we can get something like e.g. x=0,y=1
					if (blit.srcOffsets[1].x == 0)
						blit.srcOffsets[1].x = 1;
					if (blit.srcOffsets[1].y == 0)
						blit.srcOffsets[1].y = 1;

					// Destination
					blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
					blit.dstSubresource.layerCount = 1;
					blit.dstSubresource.mipLevel = i;
					blit.dstOffsets[1].x = static_cast<int32_t>(tex->GetWidth() >> i);
					blit.dstOffsets[1].y = static_cast<int32_t>(tex->GetHeight() >> i);
					blit.dstOffsets[1].z = 1;

					if (blit.dstOffsets[1].x == 0)
						blit.dstOffsets[1].x = 1;
					if (blit.dstOffsets[1].y == 0)
						blit.dstOffsets[1].y = 1;

					// Transition current mip level to transfer dst
					base.TransitionImageLayout(transferCommandBuffer, tex, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, i);
					vkCmdBlitImage(transferCommandBuffer, tex->GetImage(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, tex->GetImage(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &blit, VK_FILTER_LINEAR);
					// Transtion the current mip level to transfer src to be able to read in the next iteration
					base.TransitionImageLayout(transferCommandBuffer, tex, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, 1, i);
				}

				// Now we have all mips but they are in transfer_src, so we need to transition to shader_read
				base.TransitionImageLayout(transferCommandBuffer, tex, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, tex->GetMipLevels());

			}
			else
			{
				base.TransitionImageLayout(transferCommandBuffer, tex, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, tex->GetMipLevels());
				const std::vector<VkBufferImageCopy> &copyRegions = tex->GetCopyRegions();
				vkCmdCopyBufferToImage(transferCommandBuffer, tex->GetStagingBuffer(), tex->GetImage(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, static_cast<uint32_t>(copyRegions.size()), copyRegions.data());

				if (tex->GetTextureParams().usedAsStorageInCompute || tex->GetTextureParams().usedAsStorageInGraphics)
					base.TransitionImageLayout(transferCommandBuffer, tex, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL, tex->GetMipLevels());
				else
					base.TransitionImageLayout(transferCommandBuffer, tex, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, tex->GetMipLevels());
			}
		}
		else
		{
			const TextureParams& params = tex->GetTextureParams();

			if (params.usedAsStorageInGraphics || params.usedAsStorageInCompute)
				base.TransitionImageLayout(transferCommandBuffer, tex, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL, tex->GetMipLevels(), 0);
			else
				base.TransitionImageLayout(transferCommandBuffer, tex, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, tex->GetMipLevels(), 0);
		}

		tex->CreateImageView();
		tex->CreateSampler();
	}

	void VKRenderer::PrepareTexture3D(VKTexture3D *tex)
	{	
		if (tex->GetCopyRegions().size() > 0)
		{
			base.TransitionImageLayout(transferCommandBuffer, tex, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
			const std::vector<VkBufferImageCopy> &copyRegions = tex->GetCopyRegions();
			vkCmdCopyBufferToImage(transferCommandBuffer, tex->GetStagingBuffer(), tex->GetImage(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, static_cast<uint32_t>(copyRegions.size()), copyRegions.data());
			base.TransitionImageLayout(transferCommandBuffer, tex, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		}
		// Used as storage, so transition to GENERAL
		else
		{
			base.TransitionImageLayout(transferCommandBuffer, tex, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);
		}

		needsTransfers = true;
	}

	void VKRenderer::UpdateDescriptorSets()
	{
		for (size_t i = 0; i < setsToUpdate[currentFrame].size(); i++)
		{
			DescriptorsUpdateInfo &info = setsToUpdate[currentFrame][i];

			for (size_t j = 0; j < info.matInst->textures.size(); j++)
			{
				Texture *tex = info.matInst->textures[j];
				const TextureParams &params = tex->GetTextureParams();

				if (tex == nullptr)
					continue;

				VkWriteDescriptorSet write = {};
				write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				write.descriptorCount = 1;

				// Check if the texture is used asstorage in the graphics pipeline or in the compute.
				// For the compute if the computeSetID is set to make sure it is a compute material
				if (params.usedAsStorageInGraphics || params.usedAsStorageInCompute && info.matInst->computeSetID != std::numeric_limits<unsigned int>::max())
					write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
				else
					write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;

				write.dstArrayElement = 0;
				write.dstBinding = static_cast<uint32_t>(j);
				write.dstSet = info.set;

				if (tex->GetType() == TextureType::TEXTURE2D)
				{
					VKTexture2D *vktex = static_cast<VKTexture2D*>(tex);

					VkDescriptorImageInfo imageInfo = {};
					if (params.usedAsStorageInGraphics || params.usedAsStorageInCompute)
					{
						imageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
						//imageInfo.sampler = VK_NULL_HANDLE;			// Check first if the image is sampled
						imageInfo.sampler = vktex->GetSampler();
					}
					else if (vktex->IsDepthTexture())
					{
						imageInfo.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
						imageInfo.sampler = vktex->GetSampler();
					}
					else
					{
						imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
						imageInfo.sampler = vktex->GetSampler();
					}

					imageInfo.imageView = vktex->GetImageView();
					
					write.pImageInfo = &imageInfo;

					vkUpdateDescriptorSets(base.GetDevice(), 1, &write, 0, nullptr);
				}
				else if (tex->GetType() == TextureType::TEXTURE3D)
				{
					VKTexture3D *vktex = static_cast<VKTexture3D*>(tex);

					VkDescriptorImageInfo imageInfo = {};
					if (params.usedAsStorageInGraphics || params.usedAsStorageInCompute)
					{
						imageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
						imageInfo.sampler = VK_NULL_HANDLE;
					}
					else
					{
						imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
						imageInfo.sampler = vktex->GetSampler();
					}

					imageInfo.imageView = vktex->GetImageViewForAllMips();
					
					write.pImageInfo = &imageInfo;

					vkUpdateDescriptorSets(base.GetDevice(), 1, &write, 0, nullptr);
				}
			}

			for (size_t j = 0; j < info.matInst->buffers.size(); j++)
			{
				Buffer *buf = info.matInst->buffers[j];

				if (buf == nullptr)
					continue;

				// TODO: Check buffer type

				VkWriteDescriptorSet write = {};
				write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				write.descriptorCount = 1;
				write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
				write.dstArrayElement = 0;
				write.dstBinding = static_cast<uint32_t>(j + info.matInst->textures.size());
				write.dstSet = info.set;

				VKBuffer* buffer = static_cast<VKBuffer*>(buf);

				VkDescriptorBufferInfo bufferInfo = {};
				bufferInfo.buffer = buffer->GetBuffer();
				bufferInfo.offset = 0;
				bufferInfo.range = VK_WHOLE_SIZE;

				write.pBufferInfo = &bufferInfo;

				vkUpdateDescriptorSets(base.GetDevice(), 1, &write, 0, nullptr);
			}
		}

		setsToUpdate[currentFrame].clear();
	}

	void VKRenderer::CreateSetForMaterialInstance(MaterialInstance *matInst, PipelineType pipeType)
	{
		// Only create the set if we have textures slots
		if (matInst->textures.size() == 0)
			return;

		VkDescriptorSetAllocateInfo setAllocInfo = {};
		setAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		setAllocInfo.descriptorPool = descriptorPool;
		setAllocInfo.descriptorSetCount = 1;

		if (pipeType == PipelineType::GRAPHICS)
			setAllocInfo.pSetLayouts = &userSetLayout;
		else if (pipeType == PipelineType::COMPUTE)
			setAllocInfo.pSetLayouts = &computeSecondsSetLayouts[computeSecondsSetLayouts.size() - 1];

		DescriptorsInfo setInfo = {};

		if (vkAllocateDescriptorSets(base.GetDevice(), &setAllocInfo, &setInfo.set[0]) != VK_SUCCESS)
		{
			Log::Print(LogLevel::LEVEL_ERROR, "Failed to allocate descriptor set\n");
			return;
		}
		if (vkAllocateDescriptorSets(base.GetDevice(), &setAllocInfo, &setInfo.set[1]) != VK_SUCCESS)
		{
			Log::Print(LogLevel::LEVEL_ERROR, "Failed to allocate descriptor set\n");
			return;
		}	
		sets.push_back(setInfo);

		if (pipeType == PipelineType::GRAPHICS)
			matInst->graphicsSetID = static_cast<unsigned int>(sets.size() - 1);
		else if (pipeType == PipelineType::COMPUTE)
			matInst->computeSetID = static_cast<unsigned int>(sets.size() - 1);
	}
}
