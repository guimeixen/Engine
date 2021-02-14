#pragma once

#include "VKBase.h"
#include "VKSwapChain.h"
#include "VKBuffer.h"
#include "Graphics\Renderer.h"
#include "VKFramebuffer.h"
#include "Graphics\UniformBufferTypes.h"

namespace Engine
{
	class VKTexture2D;

	enum class PipelineType
	{
		GRAPHICS,
		COMPUTE,
	};

	struct VKBufferInfo
	{
		VkBuffer buffer;
		unsigned int binding;
	};
	struct VKImageInfo
	{
		std::vector<VkImageView> imageViews;
		VkSampler sampler;
		VkImageLayout layout;
		unsigned int index;
		bool separateMipViews;
		uint32_t mips;
	};

	class VKRenderer : public Renderer
	{
	public:
		VKRenderer(FileManager *fileManager, GLFWwindow *window, unsigned int width, unsigned int height, unsigned int monitorWidth, unsigned int monitorHeight);
		~VKRenderer();

		bool Init() override;
		void PostLoad() override;
		void Resize(unsigned int width, unsigned int height) override;
		void SetCamera(Camera *camera, const glm::vec4 &clipPlane = glm::vec4(0.0f)) override;
		void UpdateFrameDataUBO(const FrameUBO& frameData) override;

		void BeginFrame() override;
		void Present() override;
		void WaitIdle() override;

		VertexArray *CreateVertexArray(const VertexInputDesc &desc, Buffer *vertexBuffer, Buffer *indexBuffer) override;
		VertexArray *CreateVertexArray(const VertexInputDesc *descs, unsigned int descCount, const std::vector<Buffer*> &vertexBuffers, Buffer *indexBuffer) override;
		Buffer *CreateVertexBuffer(const void *data, unsigned int size, BufferUsage usage) override;
		Buffer *CreateIndexBuffer(const void *data, unsigned int size, BufferUsage usage) override;
		Buffer *CreateUniformBuffer(const void *data, unsigned int size) override;
		Buffer *CreateDrawIndirectBuffer(unsigned int size, const void *data) override;
		Buffer *CreateSSBO(unsigned int size, const void *data, unsigned int stride, BufferUsage usage) override;
		Framebuffer *CreateFramebuffer(const FramebufferDesc &desc) override;

		Shader *CreateShader(const std::string &vertexName, const std::string &fragmentName, const std::string &defines, const std::vector<VertexInputDesc> &descs, const BlendState &blendState) override;
		Shader *CreateShader(const std::string &vertexName, const std::string &fragmentName, const std::vector<VertexInputDesc> &descs, const BlendState &blendState) override;
		Shader *CreateShaderWithGeometry(const std::string &vertexPath, const std::string &geometryPath, const std::string &fragmentPath, const std::string &defines, const std::vector<VertexInputDesc> &descs) override;
		Shader *CreateShaderWithGeometry(const std::string &vertexPath, const std::string &geometryPath, const std::string &fragmentPath, const std::vector<VertexInputDesc> &descs) override;
		Shader *CreateComputeShader(const std::string &computePath, const std::string &defines) override;
		Shader *CreateComputeShader(const std::string &computePath) override;

		MaterialInstance *CreateMaterialInstance(ScriptManager &scriptManager, const std::string &matInstPath, const std::vector<VertexInputDesc> &inputDescs) override;
		MaterialInstance *CreateMaterialInstanceFromBaseMat(ScriptManager &scriptManager, const std::string &baseMatPath, const std::vector<VertexInputDesc> &inputDescs) override;

		Texture *CreateTexture2D(const std::string &path, const TextureParams &params, bool storeTextureData = false) override;
		Texture *CreateTexture3D(const std::string &path, const void *data, unsigned int width, unsigned int height, unsigned int depth, const TextureParams &params) override;
		Texture *CreateTextureCube(const std::vector<std::string> &faces, const TextureParams &params) override;
		Texture *CreateTextureCube(const std::string &path, const TextureParams &params) override;
		Texture *CreateTexture2DFromData(unsigned int width, unsigned int height, const TextureParams &params, const void *data) override;
		Texture *CreateTexture3DFromData(unsigned int width, unsigned int height, unsigned int depth, const TextureParams &params, const void *data) override;

		void SetDefaultRenderTarget() override;
		void SetRenderTarget(Framebuffer *rt) override;
		void SetRenderTargetAndClear(Framebuffer *rt) override;
		void EndRenderTarget(Framebuffer *rt) override;
		void EndDefaultRenderTarget() override;
		void ClearRenderTarget(Framebuffer *rt) override;
		void SetViewport(const Viewport &viewport) override;
		void Submit(const RenderQueue &renderQueue) override;
		void Submit(const RenderItem &renderItem) override;
		void SubmitIndirect(const RenderItem &renderItem, Buffer *indirectBuffer) override;

		void Dispatch(const DispatchItem &item) override;

		void AddTextureResourceToSlot(unsigned int binding, Texture *texture, bool useStorage, unsigned int stages, bool separateMipViews = false) override;
		void AddBufferResourceToSlot(unsigned int binding, Buffer *buffer, unsigned int stages) override;
		void SetupResources() override;
		void UpdateTextureResourceOnSlot(unsigned int binding, Texture *texture, bool useStorage, bool separateMipViews = false) override;

		void PerformBarrier(const Barrier &barrier) override;

		void CopyImage(Texture *src, Texture *dst) override;
		void ClearImage(Texture *tex) override;

		void UpdateMaterialInstance(MaterialInstance *matInst) override;

		const VKBase &GetBase() const { return base; }
		VKBase &GetBase() { return base; }
		VkDescriptorPool GetDescriptorPool() const { return descriptorPool; }
		VkRenderPass GetDefaultRenderPass() const { return defaultRenderPass; }
		VkCommandBuffer GetCurrentCommamdBuffer() const { return frameResources[currentFrame].frameCmdBuffer; }

	private:
		void Dispose() override;
		void DisposeStagingResources();

		void CreateVertexInputState(const std::vector<VertexInputDesc> &descs, std::vector<VkVertexInputBindingDescription> &bindings, std::vector<VkVertexInputAttributeDescription> &attribs);
		void CreatePipeline(ShaderPass &p, MaterialInstance *mat);
		bool CreateDefaultRenderPass();
		void PrepareTexture2D(VKTexture2D *tex);
		void PrepareTexture3D(VKTexture3D *tex);
		void UpdateDescriptorSets();
		void CreateSetForMaterialInstance(MaterialInstance *matInst, PipelineType pipeType);

	private:
		VKBase base;
		VKSwapChain swapChain;
		bool wasResized = false;
		unsigned int monitorWidth;
		unsigned int monitorHeight;

		const unsigned int MAX_CAMERAS = 16;

		static const int MAX_FRAMES_IN_FLIGHT = 2;
		struct FrameResources
		{
			VkSemaphore imageAvailableSemaphore;
			VkSemaphore renderFinishedSemaphore;
			VkFence frameFence;
			//VkFence computeFence;
			VkCommandBuffer frameCmdBuffer;
			//VkCommandBuffer computeCmdBuffer;
			VkDescriptorSet globalBuffersSet;
		};
		struct DescriptorsInfo
		{
			VkDescriptorSet set[MAX_FRAMES_IN_FLIGHT];
			unsigned int descUpdateIndex[MAX_FRAMES_IN_FLIGHT];
		};
		struct DescriptorsUpdateInfo
		{
			MaterialInstance *matInst;
			VkDescriptorSet set;
		};
		std::vector<DescriptorsInfo> sets;
		std::vector<DescriptorsUpdateInfo> setsToUpdate[2];

		FrameResources frameResources[MAX_FRAMES_IN_FLIGHT];
		VkRenderPassBeginInfo backBufferRenderPassInfos;
		unsigned int currentFrame = 0;
		uint32_t imageIndex = 0;
		VkCommandBuffer transferCommandBuffer;
		bool needsTransfers = false;

		VkRenderPass defaultRenderPass;
		bool useDefaultFramebuffer = true;
		VKFramebuffer *currentFB;
		
		std::vector<VKFramebuffer*> framebuffers;
		std::vector<VKBuffer*> vertexBuffers;
		std::vector<VKBuffer*> indexBuffers;
		std::vector<VKBuffer*> ubos;
		std::vector<VKBuffer*> drawIndirectBufs;
		std::vector<VKBuffer*> ssbos;
		std::vector<VkPipeline> pipelines;

		//std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings;
		//std::vector<VkWriteDescriptorSet> globalSetWrites;
		
		VKBuffer* cameraUBO;
		VKBuffer* frameDataUBO;

		glm::mat4* cameraUBOData;
		unsigned int currentCamera;
		unsigned int singleCameraAlignedSize;
		unsigned int allCamerasAlignedSize;

		FrameUBO frameData;
		unsigned int singleFrameUBOAlignedSize;

		VKBuffer* instanceDataSSBO;
		unsigned int instanceDataOffset = 0;
		char *mappedInstanceData;
		
		VkPipeline curPipeline;
		VkPipelineLayout graphicsPipelineLayout;
		std::vector<VkPipelineLayout> computePipelineLayouts;
		std::vector<VkDescriptorSetLayout> computeSecondsSetLayouts;

		VkDescriptorPool descriptorPool;
		std::vector<VkDescriptorSetLayoutBinding> globalBuffersSetLayoutBindings;
		std::vector<VkDescriptorSetLayoutBinding> globalTexturesSetLayoutBindings;
		VkDescriptorSetLayout globalBuffersSetLayout;
		VkDescriptorSetLayout globalTexturesSetLayout;
		VkDescriptorSetLayout userSetLayout;
		std::vector<VkWriteDescriptorSet> globalBuffersSetWrites;
		std::vector<VkWriteDescriptorSet> globalTexturesSetWrites;
		VkDescriptorSet globalTexturesSet;

		std::vector<VKBufferInfo> bufferInfos;
		std::vector<VKImageInfo> imagesInfo;

		uint32_t curMeshParamsOffset = 0;

		GLFWwindow *window;
		unsigned int textureID = 0;
	};
}
