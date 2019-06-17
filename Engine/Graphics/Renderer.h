#pragma once

#include "Camera/Camera.h"
#include "Font.h"
#include "Framebuffer.h"
#include "Lights.h"
#include "RendererStructs.h"
#include "Physics/BoundingVolumes.h"
#include "Mesh.h"
#include "MaterialInfo.h"
#include "UniformBufferTypes.h"

struct GLFWwindow;

namespace Engine
{
	class FileManager;

	enum class GraphicsAPI : unsigned short
	{
		OpenGL,
		Vulkan,
		D3D11,
		GXM
	};

	struct RenderStats
	{
		unsigned int drawCalls;
		unsigned int dispatchCalls;
		unsigned int textureChanges;
		unsigned int shaderChanges;
		unsigned int vaoChanges;
		unsigned int cullingChanges;
		unsigned int instanceCount;
		unsigned int triangles;
	};

	enum PipelineStage
	{
		INDIRECT = 1,
		VERTEX = (1 << 1),
		GEOMETRY = (1 << 2),
		FRAGMENT = (1 << 3),
		DEPTH_STENCIL_WRITE = (1 << 4),
		COMPUTE = (1 << 5),
	};

	struct BarrierImage
	{
		Texture *image;
		bool readToWrite;
		bool transitionToShaderRead;
		bool transitionToGeneral;
		unsigned int baseMip;
		unsigned int numMips;
	};

	struct BarrierBuffer
	{
		Buffer *buffer;
		bool readToWrite;
	};

	struct Barrier
	{
		unsigned int srcStage;
		unsigned int dstStage;
		std::vector<BarrierImage> images;
		std::vector<BarrierBuffer> buffers;
	};

	class Buffer;
	
	class Renderer
	{
	public:
		virtual ~Renderer() {}

		virtual bool Init() = 0;
		virtual void PostLoad() = 0;
		virtual void Resize(unsigned int width, unsigned int height) = 0;
		virtual void SetCamera(Camera *camera, const glm::vec4 &clipPlane = glm::vec4(0.0f)) = 0;
		virtual void SetViewportPos(const glm::ivec2 &pos) { viewportPos = pos; }
		virtual void BeginFrame() {}
		virtual void Present() {}
		virtual void WaitIdle() {}

		virtual VertexArray *CreateVertexArray(const VertexInputDesc &desc, Buffer *vertexBuffer, Buffer *indexBuffer) = 0;
		virtual VertexArray *CreateVertexArray(const VertexInputDesc *descs, unsigned int descCount, const std::vector<Buffer*> &vertexBuffers, Buffer *indexBuffer) = 0;
		virtual Buffer *CreateVertexBuffer(const void *data, unsigned int size, BufferUsage usage) = 0;
		virtual Buffer *CreateIndexBuffer(const void *data, unsigned int size, BufferUsage usage) = 0;
		virtual Buffer *CreateUniformBuffer(const void *data, unsigned int size) = 0;
		virtual Buffer *CreateDrawIndirectBuffer(unsigned int size, const void *data) = 0;
		virtual Buffer *CreateSSBO(unsigned int size, const void *data, unsigned int stride, BufferUsage usage) = 0;
		virtual Framebuffer *CreateFramebuffer(const FramebufferDesc &desc) = 0;

		virtual Shader *CreateShader(const std::string &vertexName, const std::string &fragmentName, const std::string &defines, const std::vector<VertexInputDesc> &descs, const BlendState &blendState) = 0;
		virtual Shader *CreateShader(const std::string &vertexName, const std::string &fragmentName, const std::vector<VertexInputDesc> &descs, const BlendState &blendState) = 0;
		virtual Shader *CreateShaderWithGeometry(const std::string &vertexPath, const std::string &geometryPath, const std::string &fragmentPath, const std::string &defines, const std::vector<VertexInputDesc> &descs) = 0;
		virtual Shader *CreateShaderWithGeometry(const std::string &vertexPath, const std::string &geometryPath, const std::string &fragmentPath, const std::vector<VertexInputDesc> &descs) = 0;
		virtual Shader *CreateComputeShader(const std::string &defines, const std::string &computePath) = 0;
		virtual Shader *CreateComputeShader(const std::string &computePath) = 0;

		virtual MaterialInstance *CreateMaterialInstance(ScriptManager &scriptManager, const std::string &matInstPath, const std::vector<VertexInputDesc> &inputDescs) = 0;
		virtual MaterialInstance *CreateMaterialInstanceFromBaseMat(ScriptManager &scriptManager, const std::string &baseMatPath, const std::vector<VertexInputDesc> &inputDescs) = 0;

		virtual void ReloadMaterial(Material *baseMaterial) = 0;

		virtual Texture *CreateTexture2D(const std::string &path, const TextureParams &params, bool storeTextureData = false) = 0;
		virtual Texture *CreateTexture3D(const std::string &path, const void *data, unsigned int width, unsigned int height, unsigned int depth, const TextureParams &params) = 0;
		virtual Texture *CreateTextureCube(const std::vector<std::string> &faces, const TextureParams &params) = 0;
		virtual Texture *CreateTextureCube(const std::string &path, const TextureParams &params) = 0;
		virtual Texture *CreateTexture2DFromData(unsigned int width, unsigned int height, const TextureParams &params, const void *data) = 0;
		virtual Texture *CreateTexture3DFromData(unsigned int width, unsigned int height, unsigned int depth, const TextureParams &params, const void *data) = 0;

		virtual void SetDefaultRenderTarget() = 0;
		virtual void SetRenderTarget(Framebuffer *rt) = 0;
		virtual void SetRenderTargetAndClear(Framebuffer *rt) = 0;
		virtual void EndRenderTarget(Framebuffer *rt) = 0;
		virtual void EndDefaultRenderTarget() = 0;
		virtual void ClearRenderTarget(Framebuffer *rt) = 0;
		virtual void SetViewport(const Viewport &viewport) = 0;
		virtual void Submit(const RenderQueue &renderQueue) = 0;
		virtual void Submit(const RenderItem &renderItem) = 0;
		virtual void SubmitIndirect(const RenderItem &renderItem, Buffer *indirectBuffer) = 0;
		virtual void Dispatch(const DispatchItem &item) = 0;

		virtual void AddResourceToSlot(unsigned int binding, Texture *texture, bool useStorage, unsigned int stages, bool separateMipViews = false) = 0;
		virtual void AddResourceToSlot(unsigned int binding, Buffer *buffer, unsigned int stages) = 0;
		virtual void SetupResources() = 0;
		virtual void UpdateResourceOnSlot(unsigned int binding, Texture *texture, bool useStorage, bool separateMipViews = false) = 0;

		virtual void PerformBarrier(const Barrier &barrier) = 0;

		virtual void BindImage(unsigned int slot, unsigned int mipLevel, Texture *tex, ImageAccess access) {}
		virtual void ClearBoundImages() {}
		virtual void CopyImage(Texture *src, Texture *dst) = 0;
		virtual void ClearImage(Texture *tex) = 0;

		std::vector<VisibilityIndices> Cull(unsigned int queueAndFrustumCount, unsigned int *queueIDs, const Frustum *frustums);
		void CopyVisibilityToQueue(std::vector<VisibilityIndices> &visibility, unsigned int srcQueueIndex, unsigned int dstQueueIndex);
		void CreateRenderQueues(unsigned int queueCount, unsigned int *queueIDs, const std::vector<VisibilityIndices> &visibility, RenderQueue *outQueues);

		virtual void UpdateMaterialInstance(MaterialInstance *matInst) = 0;

		virtual void RebindTexture(Texture *texture) {}

		void SetFrameTime(float frameTime) { this->frameTime = frameTime; }
		float GetFrameTime() const { return frameTime; }

		void AddRenderQueueGenerator(RenderQueueGenerator *renderQueueGenerator) { renderQueueGenerators.push_back(renderQueueGenerator); }
		void RemoveRenderQueueGenerator(RenderQueueGenerator *generator);
		bool RemoveTexture(Texture *t);
		bool RemoveMaterialInstance(MaterialInstance *m);			// Probably should remove this function

		unsigned int GetWidth() const { return width; }
		unsigned int GetHeight() const { return height; }
		Camera *GetCamera() const { return camera; }
		const RenderStats &GetRendererStats() const { return renderStats; }
		FileManager *GetFileManager() const { return fileManager; }

		void AddGlobalDefine(const std::string &define) { globalDefines += define + '\n'; }
		const std::string &GetGlobalDefines() const { return globalDefines; }

		static Renderer *Create(GLFWwindow *window, GraphicsAPI api, FileManager *fileManager, unsigned int width, unsigned int height, unsigned int monitorWidth, unsigned int monitorHeight);
		static GraphicsAPI GetCurrentAPI() { return currentAPI; };
		static unsigned int GetBlendFactorValue(BlendFactor blendFactor);
		static unsigned int GetTopologyValue(Topology topology);
		static unsigned int GetDepthFunc(const std::string &func);
		static unsigned int GetCullMode(const std::string &mode);
		static unsigned int GetFrontFace(const std::string &face);

	private:
		virtual void Dispose() = 0;

	protected:
		static GraphicsAPI currentAPI;

		FileManager *fileManager;

		unsigned int width;
		unsigned int height;
		glm::ivec2 viewportPos;
		RenderStats renderStats;
		float frameTime = 0.0f;

		Camera *camera;
		Material *currentMaterial;

		std::vector<RenderQueueGenerator*> renderQueueGenerators;
		std::vector<MaterialInstance*> materialInstances;
		std::map<unsigned int, Shader*> shaders;
		std::map<unsigned int, Texture*> textures;

		std::string globalDefines;

		Buffer *meshParamsUBO = nullptr;
		std::vector<const void*> meshParamsData;
		unsigned int meshParamsOffset = 0;
		void *buffer[16000];

		unsigned int instanceDataOffset = 0;
		std::vector<const void*> instanceData;
		void *instanceBuffer[16000];	
	};
}
