#pragma once

#include "Graphics/Renderer.h"
#include "psp2/gxm.h"

namespace Engine
{
	class GXMFramebuffer;
	class GXMVertexBuffer;
	class GXMIndexBuffer;
	class GXMShader;
	class GXMTexture2D;
	class GXMUniformBuffer;

	class GXMRenderer : public Renderer
	{
	public:
		GXMRenderer(FileManager *fileManager);
		~GXMRenderer();

		bool Init() override;
		void PostLoad() override;
		void Resize(unsigned int width, unsigned int height) override;
		void SetCamera(Camera *camera, const glm::vec4 &clipPlane = glm::vec4(0.0f)) override;

		void Present();

		VertexArray *CreateVertexArray(const VertexInputDesc &desc, Buffer *vertexBuffer, Buffer *indexBuffer) override;
		VertexArray *CreateVertexArray(const VertexInputDesc *descs, unsigned int descCount, const std::vector<Buffer*> &vertexBuffers, Buffer *indexBuffer) override;
		Buffer *CreateVertexBuffer(const void *data, unsigned int size, BufferUsage usage) override;
		Buffer *CreateIndexBuffer(const void *data, unsigned int size, BufferUsage usage) override;
		Buffer *CreateUniformBuffer(const void *data, unsigned int size) override;
		Buffer *CreateDrawIndirectBuffer(unsigned int size, const void *data) override { return nullptr; }
		Buffer *CreateSSBO(unsigned int size, const void *data, unsigned int stride, BufferUsage usage) override { return nullptr; }
		Framebuffer *CreateFramebuffer(const FramebufferDesc &desc) override;

		Shader *CreateShader(const std::string &vertexName, const std::string &fragmentName, const std::string &defines, const std::vector<VertexInputDesc> &descs, const BlendState &blendState) override;
		Shader *CreateShader(const std::string &vertexName, const std::string &fragmentName, const std::vector<VertexInputDesc> &descs, const BlendState &blendState) override;
		Shader *CreateShaderWithGeometry(const std::string &vertexPath, const std::string &geometryPath, const std::string &fragmentPath, const std::string &defines, const std::vector<VertexInputDesc> &descs) override { return nullptr; }
		Shader *CreateShaderWithGeometry(const std::string &vertexPath, const std::string &geometryPath, const std::string &fragmentPath, const std::vector<VertexInputDesc> &descs) override { return nullptr; }
		Shader *CreateComputeShader(const std::string &defines, const std::string &computePath) override { return nullptr; }
		Shader *CreateComputeShader(const std::string &computePath) override { return nullptr; }

		MaterialInstance *CreateMaterialInstance(ScriptManager &scriptManager, const std::string &matInstPath, const std::vector<VertexInputDesc> &inputDescs) override;
		MaterialInstance *CreateMaterialInstanceFromBaseMat(ScriptManager &scriptManager, const std::string &baseMatPath, const std::vector<VertexInputDesc> &inputDescs) override;

		Texture *CreateTexture2D(const std::string &path, const TextureParams &params, bool storeTextureData = false) override;
		Texture *CreateTexture3D(const std::string &path, const void *data, unsigned int width, unsigned int height, unsigned int depth, const TextureParams &params) override { return nullptr; }
		Texture *CreateTextureCube(const std::vector<std::string> &faces, const TextureParams &params) override;
		Texture *CreateTextureCube(const std::string &path, const TextureParams &params) override;
		Texture *CreateTexture2DFromData(unsigned int width, unsigned int height, const TextureParams &params, const void *data) override;
		Texture *CreateTexture3DFromData(unsigned int width, unsigned int height, unsigned int depth, const TextureParams &params, const void *data) override { return nullptr; }

		void SetDefaultRenderTarget() override;
		void SetRenderTarget(Framebuffer *rt) override {}
		void SetRenderTargetAndClear(Framebuffer *rt) override;
		void EndRenderTarget(Framebuffer *rt) override;
		void EndDefaultRenderTarget() override;
		void ClearRenderTarget(Framebuffer *rt) override;
		void SetViewport(const Viewport &viewport) override;
		void Submit(const RenderQueue &renderQueue) override;
		void Submit(const RenderItem &renderItem) override;
		void SubmitIndirect(const RenderItem &renderItem, Buffer *indirectBuffer) override {}
		void Dispatch(const DispatchItem &item) override {}

		void AddResourceToSlot(unsigned int binding, Texture *texture, bool useStorage, unsigned int stages, bool separateMipViews = false) override;
		void AddResourceToSlot(unsigned int binding, Buffer *buffer, unsigned int stages) override;
		void SetupResources() override;
		void UpdateResourceOnSlot(unsigned int binding, Texture *texture, bool useStorage, bool separateMipViews = false) override;

		void PerformBarrier(const Barrier &barrier) override;

		void CopyImage(Texture *src, Texture *dst) override {}
		void ClearImage(Texture *tex) override {}

		void UpdateMaterialInstance(MaterialInstance *matInst) override;

	private:
		void SetDepthStencilState(const DepthStencilState &state);
		void SetRasterizerState(const RasterizerState &state);
		void Dispose() override;
		static void DisplayQueueCallback(const void *callbackData);
		static void *ShaderPatcherHostAllocCb(void *user_data, unsigned int size);
		static void ShaderPatcherHostFreeCb(void *user_data, void *mem);

	private:
		SceGxmContext *context;
		SceGxmContextParams contextParams;

		SceUID vdmRingBufferUID;
		void *vdmRingBufferAddr;
		SceUID vertexRingBufferUID;
		void *vertexRingBufferAddr;
		SceUID fragmentRingBufferUID;
		void *fragmentRingBufferAddr;
		SceUID fragmentUsseRingBufferUID;
		void *fragmentUsseRingBufferAddr;
		SceGxmShaderPatcher *shaderPatcher;
		SceUID shaderPatcherBufferUID;
		void *shaderPatcherBufferAddr;
		SceUID shaderPatcherBufferVertexUsseUID;
		void *shaderPatcherBufferVertexUsseAddr;
		SceUID shaderPatcherBufferFragmentUsseUID;
		void *shaderPatcherBufferFragmentUsseAddr;

		unsigned int backBufferIndex;
		unsigned int frontBufferIndex;
		unsigned int numVertexTextureBindings;
		unsigned int numFragmentTextureBindings;

		unsigned int depthBiasFactor;
		unsigned int depthBiasUnits;

		GXMShader *clearShader;
		const SceGxmProgramParameter *paramColorUniform;

		GXMVertexBuffer *clearVB;
		GXMIndexBuffer *clearIB;

		SceGxmVertexProgram *currentVertexProgram;
		SceGxmFragmentProgram *currentFragmentProgram;

		DepthStencilState depthStencilState;
		RasterizerState rasterizerState;

		GXMTexture2D *texture2d;

		GXMUniformBuffer *ubo[2];
		float b = 1.0f;
		float time = 0.0f;

		static const unsigned int MAX_CAMERAS = 4;
		GXMUniformBuffer *cameraUBOs[MAX_CAMERAS];
		unsigned int currentCameraIndex = 0;
		unsigned int availableCameras = MAX_CAMERAS;

		SceGxmNotification vertexNotif[2];
		SceGxmNotification fragmentNotif[2];
		unsigned int vertexIndex = 0;
		unsigned int fragmentIndex = 0;

		void *clearColorSurfaceData[2];
		void *clearDepthSurfaceData;
		SceGxmColorSurface clearColorSurfaces[2];
		SceGxmDepthStencilSurface clearDepthStencilSurface;
		SceUID clearColorDataUIDs[2];
		SceUID clearDepthDataUID;
		SceGxmSyncObject *clearSyncObjs[2];
		SceGxmRenderTarget *defaultRenderTarget;
	};
}
