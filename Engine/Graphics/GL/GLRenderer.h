#pragma once

#include "include/glew/glew.h"

#include "Graphics/Renderer.h"
#include "Graphics/GL/GLFramebuffer.h"
#include "GLVertexArray.h"
#include "GLUniformBuffer.h"

namespace Engine
{
	class Buffer;

	class GLRenderer : public Renderer
	{
	public:
		GLRenderer(FileManager *fileManager, GLuint width, GLuint height);
		~GLRenderer();

		bool Init() override;
		void PostLoad() override;
		void Resize(unsigned int width, unsigned int height) override;
		void SetCamera(Camera *camera, const glm::vec4 &clipPlane = glm::vec4(0.0f)) override;
		void UpdateFrameDataUBO(const FrameUBO& frameData) override;

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
		Shader *CreateComputeShader(const std::string &defines, const std::string &computePath) override;
		Shader *CreateComputeShader(const std::string &computePath) override;

		MaterialInstance *CreateMaterialInstance(ScriptManager &scriptManager, const std::string &matInstPath, const std::vector<VertexInputDesc> &inputDescs) override;
		MaterialInstance *CreateMaterialInstanceFromBaseMat(ScriptManager &scriptManager, const std::string &baseMatPath, const std::vector<VertexInputDesc> &inputDescs) override;

		Texture *CreateTexture2D(const std::string &path, const TextureParams &params, bool storeTextureData = false) override;
		Texture *CreateTexture3D(const std::string &path, const void *data, unsigned int width, unsigned int height, unsigned int depth, const TextureParams &params) override;
		Texture *CreateTextureCube(const std::vector<std::string> &faces, const TextureParams &params) override;
		Texture *CreateTextureCube(const std::string &path, const TextureParams &params) override;
		Texture *CreateTexture2DFromData(unsigned int width, unsigned int height, const TextureParams &params, const void *data) override;
		Texture *CreateTexture3DFromData(unsigned int width, unsigned int height, unsigned int depth, const TextureParams &params, const void *data) override;

		void BeginFrame() override;
		void SetDefaultRenderTarget() override;
		void SetRenderTarget(Framebuffer *rt) override;
		void SetRenderTargetAndClear(Framebuffer *rt) override;
		void EndRenderTarget(Framebuffer *rt) override {}
		void EndDefaultRenderTarget() override {}
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

		void BindImage(unsigned int slot, unsigned int mipLevel, Texture *tex, ImageAccess access);
		void CopyImage(Texture *src, Texture *dst) override;
		void ClearImage(Texture *tex) override;

		void UpdateMaterialInstance(MaterialInstance *matInst) override;

	private:
		void SortCommands();

		void SetBlendState(const BlendState &state);
		void SetDepthStencilState(const DepthStencilState &state);
		void SetRasterizerState(const RasterizerState &state);
		void SetShader(unsigned int shader);
		void SetTexture(unsigned int id, unsigned int slot);

		void Dispose() override;

	private:
		unsigned int uboMinOffsetAlignment;
		GLUniformBuffer *viewUniformBuffer = nullptr;
		GLUniformBuffer *materialUBO;

		GLuint instanceDataSSBO;

		BlendState blendState;
		DepthStencilState depthStencilState;
		RasterizerState rasterizerState;
		unsigned int currentShader;

		unsigned int currentTextures[16];

		unsigned int currentTextureBinding = 0;

		//Buffer* meshParamsUBO = nullptr;
		//std::vector<const void*> meshParamsData;
		//unsigned int meshParamsOffset = 0;
		//void* buffer[16000];

		unsigned int instanceDataOffset = 0;
		std::vector<const void*> instanceData;
		void* instanceBuffer[16000];
	};
}
