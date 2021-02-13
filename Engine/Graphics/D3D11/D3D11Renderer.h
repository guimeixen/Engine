#pragma once

#include "Graphics\Renderer.h"
#include "D3D11UniformBuffer.h"

#include <d3d11_1.h>

namespace Engine
{
	struct DispatchParams
	{
		glm::uvec3 numWorkGroups;
		unsigned int pad0;
	};

	class D3D11Shader;
	class D3D11SSBO;
	class D3D11DrawIndirectBuffer;

	class D3D11Renderer : public Renderer
	{
	public:
		D3D11Renderer(HWND hwnd, unsigned int width, unsigned int height);
		~D3D11Renderer();

		bool Init() override;
		void PostLoad() override;
		void Resize(unsigned int width, unsigned int height) override;
		void SetCamera(Camera *camera, const glm::vec4 &clipPlane = glm::vec4(0.0f)) override;
		void UpdateFrameDataUBO(const FrameUBO& frameData) override;

		void BeginFrame() override;
		void Present() override;

		VertexArray *CreateVertexArray(const VertexInputDesc &desc, Buffer *vertexBuffer, Buffer *indexBuffer) override;
		VertexArray *CreateVertexArray(const VertexInputDesc *descs, unsigned int descCount, const std::vector<Buffer*> &vertexBuffers, Buffer *indexBuffer) override;
		Buffer *CreateVertexBuffer(const void *data, unsigned int size, BufferUsage usage) override;
		Buffer *CreateIndexBuffer(const void *data, unsigned int size, BufferUsage usage) override;
		Buffer *CreateUniformBuffer(const void *data, unsigned int size) override;
		Buffer *CreateDrawIndirectBuffer(unsigned int size, const void *data) override;
		Buffer *CreateSSBO(unsigned int size, const void *data, unsigned int stride, BufferUsage usage) override;
		Framebuffer *CreateFramebuffer(const FramebufferDesc &desc) override;

		Shader *CreateShader(const std::string &vertexName, const std::string &fragmentName, const std::string &defines, const std::vector<VertexInputDesc> &descs, const BlendState &blendState)  override;
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

		void BindImage(unsigned int slot, unsigned int mipLevel, Texture *tex, ImageAccess access);
		void ClearBoundImages();
		void CopyImage(Texture *src, Texture *dst) override;
		void ClearImage(Texture *tex) override;

		void UpdateMaterialInstance(MaterialInstance *matInst) override;

		void RebindTexture(Texture *texture);

		ID3D11Device *GetDevice() const { return device; }
		ID3D11DeviceContext *GetContext() const { return immediateContext; }

	private:
		void Dispose() override{}
		void CreateState(const ShaderPass &pass);
		bool CreateBackBufferRTVAndDSV();
		void ClearShaderResources();

	private:
		HWND hwnd;
		ID3D11Device *device;
		ID3D11DeviceContext *immediateContext = nullptr;
		IDXGISwapChain *swapChain = nullptr;
		ID3D11RenderTargetView *renderTargetView = nullptr;
		ID3D11Texture2D *depthStencilTexture = nullptr;
		ID3D11DepthStencilView *depthStencilView = nullptr;
		float clearColor[4];

		std::vector<Framebuffer*> framebuffers;
		std::vector<ID3D11ShaderResourceView*> shaderResources;
		std::vector<ID3D11SamplerState*> samplerStates;
		std::vector<ID3D11ShaderResourceView*> nullSRVs;
		std::vector<ID3D11UnorderedAccessView*> nullUAVs;

		D3D11UniformBuffer *cameraUBO = nullptr;
		D3D11UniformBuffer *materialDataUBO = nullptr;
		D3D11UniformBuffer *dispatchParamsUBO = nullptr;
		D3D11Shader *currentShader = nullptr;

		struct State
		{
			ID3D11RasterizerState *rsState;
			ID3D11DepthStencilState *dsState;
			ID3D11BlendState *blendState;
			Material *mat;
		};
		std::vector<State> states;
		unsigned int currentStateID = 0;
		glm::mat4 correctTransform;
		unsigned int textureID = 0;

		std::vector<ID3D11ShaderResourceView*> setSRVs;
		std::vector<ID3D11SamplerState*> setSamplers;

		std::vector<D3D11UniformBuffer*> ubos;
		std::vector<D3D11SSBO*> ssbos;
		std::vector<D3D11DrawIndirectBuffer*> indirectBuffers;
		unsigned int setSRVCount = 0;
		unsigned int setUAVs = 0;

		unsigned int currentCBBinding = 0;
		unsigned int currentSRVBinding = 0;
		unsigned int currentUAVBinding = 0;
		unsigned int currentSampler = 0;

		D3D11SSBO *instanceDataSSBO;
		unsigned int instanceDataOffset;

		unsigned int currentTopology = -1;

		std::vector<Texture*> imagesToBindUAVs;
	};
}
