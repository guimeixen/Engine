#include "GXMRenderer.h"

#include "psp2/display.h"
#include "GXMUtils.h"
#include "GXMFramebuffer.h"
#include "GXMVertexArray.h"
#include "GXMVertexBuffer.h"
#include "GXMIndexBuffer.h"
#include "GXMTexture2D.h"
#include "GXMUniformBuffer.h"
#include "GXMShader.h"
#include "Program/Log.h"
#include "Program/StringID.h"
#include "Graphics/Material.h"

#include "include/glm/gtc/matrix_transform.hpp"

#define DISPLAY_WIDTH 960
#define DISPLAY_HEIGHT 544
#define DISPLAY_STRIDE_IN_PIXELS	 1024
#define DISPLAY_BUFFER_COUNT 2
#define DISPLAY_COLOR_FORMAT SCE_GXM_COLOR_FORMAT_A8B8G8R8
#define DISPLAY_PIXEL_FORMAT SCE_DISPLAY_PIXELFORMAT_A8B8G8R8
#define MAX_PENDING_SWAPS (DISPLAY_BUFFER_COUNT - 1)

struct displayQueueCallbackData
{
	void *addr;
};

namespace Engine
{
	GXMRenderer::GXMRenderer(FileManager *fileManager)
	{
		this->fileManager = fileManager;
	}

	GXMRenderer::~GXMRenderer()
	{
		Dispose();
	}

	bool GXMRenderer::Init()
	{
		width = DISPLAY_WIDTH;
		height = DISPLAY_HEIGHT;

		currentAPI = GraphicsAPI::GXM;

		int err = 0;

		// Init GXM
		SceGxmInitializeParams gxmInitParams = {};
		gxmInitParams.flags = 0;
		gxmInitParams.displayQueueMaxPendingCount = MAX_PENDING_SWAPS;
		gxmInitParams.displayQueueCallback = GXMRenderer::DisplayQueueCallback;
		gxmInitParams.displayQueueCallbackDataSize = sizeof(displayQueueCallbackData);
		gxmInitParams.parameterBufferSize = SCE_GXM_DEFAULT_PARAMETER_BUFFER_SIZE;

		err = sceGxmInitialize(&gxmInitParams);
		if (err)
		{
			Log::Print(LogLevel::LEVEL_INFO, "Failed to initialize GXM: %d\n", err);
			return false;
		}
		Log::Print(LogLevel::LEVEL_INFO, "GXM Initialized\n");

		vdmRingBufferAddr = gxmutils::graphicsAlloc(SCE_KERNEL_MEMBLOCK_TYPE_USER_RW_UNCACHE, SCE_GXM_MEMORY_ATTRIB_READ, SCE_GXM_DEFAULT_VDM_RING_BUFFER_SIZE, &vdmRingBufferUID);
		vertexRingBufferAddr = gxmutils::graphicsAlloc(SCE_KERNEL_MEMBLOCK_TYPE_USER_RW_UNCACHE, SCE_GXM_MEMORY_ATTRIB_READ, SCE_GXM_DEFAULT_VERTEX_RING_BUFFER_SIZE, &vertexRingBufferUID);
		fragmentRingBufferAddr = gxmutils::graphicsAlloc(SCE_KERNEL_MEMBLOCK_TYPE_USER_RW_UNCACHE, SCE_GXM_MEMORY_ATTRIB_READ, SCE_GXM_DEFAULT_FRAGMENT_RING_BUFFER_SIZE, &fragmentRingBufferUID);

		unsigned int fragmentUsseOffset = 0;
		fragmentUsseRingBufferAddr = gxmutils::gpuFragmentUsseAllocMap(SCE_GXM_DEFAULT_FRAGMENT_USSE_RING_BUFFER_SIZE, &fragmentUsseRingBufferUID, &fragmentUsseOffset);

		// Create context
		contextParams = {};
		contextParams.hostMem = malloc(SCE_GXM_MINIMUM_CONTEXT_HOST_MEM_SIZE);
		contextParams.hostMemSize = SCE_GXM_MINIMUM_CONTEXT_HOST_MEM_SIZE;
		contextParams.vdmRingBufferMem = vdmRingBufferAddr;
		contextParams.vdmRingBufferMemSize = SCE_GXM_DEFAULT_VDM_RING_BUFFER_SIZE;
		contextParams.vertexRingBufferMem = vertexRingBufferAddr;
		contextParams.vertexRingBufferMemSize = SCE_GXM_DEFAULT_VERTEX_RING_BUFFER_SIZE;
		contextParams.fragmentRingBufferMem = fragmentRingBufferAddr;
		contextParams.fragmentRingBufferMemSize = SCE_GXM_DEFAULT_FRAGMENT_RING_BUFFER_SIZE;
		contextParams.fragmentUsseRingBufferMem = fragmentUsseRingBufferAddr;
		contextParams.fragmentUsseRingBufferMemSize = SCE_GXM_DEFAULT_FRAGMENT_USSE_RING_BUFFER_SIZE;
		contextParams.fragmentUsseRingBufferOffset = fragmentUsseOffset;

		sceGxmCreateContext(&contextParams, &context);
		Log::Print(LogLevel::LEVEL_INFO, "Context created\n");

		FramebufferDesc desc = {};
		desc.width = DISPLAY_WIDTH;
		desc.height = DISPLAY_HEIGHT;
		desc.useDepth = true;

		TextureParams p = {};
		p.filter = TextureFilter::LINEAR;
		p.format = TextureFormat::RGBA;
		p.internalFormat = TextureInternalFormat::RGBA8;
		p.type = TextureDataType::UNSIGNED_BYTE;
		p.wrap = TextureWrap::CLAMP_TO_EDGE;

		for (int i = 0; i < DISPLAY_BUFFER_COUNT; i++)
		{
			desc.colorTextures.push_back(p);
		}

		TextureParams dp = {};
		dp.filter = TextureFilter::NEAREST;
		dp.format = TextureFormat::DEPTH_COMPONENT;
		dp.internalFormat = TextureInternalFormat::DEPTH_COMPONENT24;
		dp.type = TextureDataType::FLOAT;
		dp.wrap = TextureWrap::CLAMP_TO_EDGE;

		desc.depthTexture = dp;

		defaultFB = new GXMFramebuffer(desc);

		// Allocate memory for buffers and USSE code
		size_t shaderPatcherBufferSize = 64 * 1024;
		const unsigned int shaderPatcherVertexUsseSize = 64 * 1024;
		const unsigned int shaderPatcherFragmentUsseSize = 64 * 1024;

		shaderPatcherBufferAddr = gxmutils::graphicsAlloc(SCE_KERNEL_MEMBLOCK_TYPE_USER_RW_UNCACHE, (SceGxmMemoryAttribFlags)(SCE_GXM_MEMORY_ATTRIB_READ | SCE_GXM_MEMORY_ATTRIB_WRITE), shaderPatcherBufferSize, &shaderPatcherBufferUID);


		unsigned int shaderPatcherVertexUsseOffset = 0;
		shaderPatcherBufferVertexUsseAddr = gxmutils::gpuVertexUsseAllocMap(shaderPatcherVertexUsseSize, &shaderPatcherBufferVertexUsseUID, &shaderPatcherVertexUsseOffset);

		unsigned int shaderPatcherFragmentUsseOffset = 0;
		shaderPatcherBufferFragmentUsseAddr = gxmutils::gpuFragmentUsseAllocMap(shaderPatcherFragmentUsseSize, &shaderPatcherBufferFragmentUsseUID, &shaderPatcherFragmentUsseOffset);

		// Create the shader patcher
		SceGxmShaderPatcherParams patcherParams = {};
		patcherParams.userData = nullptr;
		patcherParams.hostAllocCallback = &GXMRenderer::ShaderPatcherHostAllocCb;
		patcherParams.hostFreeCallback = &GXMRenderer::ShaderPatcherHostFreeCb;
		patcherParams.bufferAllocCallback = nullptr;
		patcherParams.bufferFreeCallback = nullptr;
		patcherParams.bufferMem = shaderPatcherBufferAddr;
		patcherParams.bufferMemSize = shaderPatcherBufferSize;
		patcherParams.vertexUsseAllocCallback = nullptr;
		patcherParams.vertexUsseFreeCallback = nullptr;
		patcherParams.vertexUsseMem = shaderPatcherBufferVertexUsseAddr;
		patcherParams.vertexUsseMemSize = shaderPatcherVertexUsseSize;
		patcherParams.vertexUsseOffset = shaderPatcherVertexUsseOffset;
		patcherParams.fragmentUsseAllocCallback = nullptr;
		patcherParams.fragmentUsseFreeCallback = nullptr;
		patcherParams.fragmentUsseMem = shaderPatcherBufferFragmentUsseAddr;
		patcherParams.fragmentUsseMemSize = shaderPatcherFragmentUsseSize;
		patcherParams.fragmentUsseOffset = shaderPatcherFragmentUsseOffset;

		//check error
		sceGxmShaderPatcherCreate(&patcherParams, &shaderPatcher);
		Log::Print(LogLevel::LEVEL_INFO, "Created shader patcher\n");

		Engine::VertexAttribute pos = {};
		pos.count = 2;
		pos.offset = 0;
		pos.vertexAttribFormat = Engine::VertexAttributeFormat::FLOAT;

		Engine::VertexInputDesc inputDesc = {};
		inputDesc.attribs.push_back(pos);
		inputDesc.stride = 2 * sizeof(float);

		BlendState b = {};
		clearShader = new GXMShader(shaderPatcher, fileManager, "color", "color", { inputDesc }, b);

		paramColorUniform = clearShader->GetParameter(false, "color");

		float clearVertices[] = {
			-1.0f, -1.0f,
			3.0f, -1.0f,
			-1.0f, 3.0f
		};
		unsigned short indices[] = { 0,1,2 };

		clearVB = new GXMVertexBuffer(clearVertices, sizeof(clearVertices), BufferUsage::STATIC);
		clearIB = new GXMIndexBuffer(indices, sizeof(indices), BufferUsage::STATIC);

		/*TextureParams params = {};
		params.format = TextureFormat::RGBA;
		texture2d = new GXMTexture2D();
		if (!texture2d->Load(fileManager, "Data/Textures/sand.png", params))
			return false;*/

		//sceGxmReserveFragmentDefaultUniformBuffer();
		//sceGxmSetFragmentUniformBuffer();
		
		//sceGxmSetUniformDataF();
		
		float myubo[] = { 0.0f, 0.0f, 1.0f };
		ubo[0] = new GXMUniformBuffer(myubo, 12, BufferUsage::STATIC);
		ubo[1] = new GXMUniformBuffer(myubo, 12, BufferUsage::STATIC);

		volatile unsigned int *const notificationMem = sceGxmGetNotificationRegion();

		vertexNotif[0].address = notificationMem;
		vertexNotif[0].value = 0;
		vertexNotif[1].address = notificationMem + 1;
		vertexNotif[1].value = 0;

		fragmentNotif[0].address = notificationMem + 2;
		fragmentNotif[0].value = 0;
		fragmentNotif[1].address = notificationMem + 3;
		fragmentNotif[1].value = 0;


		glm::mat4 projView = glm::perspective(75.0f, 960.0f / 544.0f, 0.1f, 10.0f) * glm::lookAt(glm::vec3(0.0f, 0.0f, 3.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));

		for (size_t i = 0; i < MAX_CAMERAS; i++)
		{
			cameraUBOs[i] = new GXMUniformBuffer(&projView[0].x, sizeof(projView), BufferUsage::STATIC);
		}

		sceGxmSetVertexUniformBuffer(context, 0, cameraUBOs[currentCameraIndex]->GetUBO());

		depthStencilState.depthEnable = true;
		//depthStencilState.depthFunc = ;
		depthStencilState.depthWrite = true;
		/*rasterizerState.enableCulling = true;
		rasterizerState.frontFace = ;
		rasterizerState.cullFace = ;*/

		Log::Print(LogLevel::LEVEL_INFO, "Done!\n");

		return true;
	}

	void GXMRenderer::PostLoad()
	{
	}

	void GXMRenderer::Resize(unsigned int width, unsigned int height)
	{
	}

	void GXMRenderer::SetCamera(Camera *camera, const glm::vec4 &clipPlane)
	{
		this->camera = camera;

		glm::mat4 proj = camera->GetProjectionMatrix();
		glm::mat4 view = camera->GetViewMatrix();

		viewProjUBO ubo = {};

		if (camera->GetFrustum().GetType() == FrustumType::ORTHOGRAPHIC)
		{
			ubo.proj = glm::transpose(proj);
			ubo.view = glm::transpose(view);
			ubo.projView = glm::transpose(proj * view);
		}
		else
		{
			ubo.proj = proj;
			ubo.view = view;
			ubo.projView = proj * view;
		}

		
		ubo.invView = glm::inverse(ubo.view);
		ubo.invProj = glm::inverse(ubo.proj);
		ubo.clipPlane = clipPlane;
		ubo.camPos = glm::vec4(camera->GetPosition(), 0.0f);
		ubo.nearFarPlane = glm::vec2(camera->GetNearPlane(), camera->GetFarPlane());

		// If we don't have more cameras available, we need to wait.
		/*if (availableCameras < 1)
		{

		}*/

		cameraUBOs[currentCameraIndex]->Update(&ubo.projView[0].x, sizeof(ubo.projView), 0);

		sceGxmSetVertexUniformBuffer(context, 0, cameraUBOs[currentCameraIndex]->GetUBO());
		sceGxmSetFragmentUniformBuffer(context, 0, cameraUBOs[currentCameraIndex]->GetUBO());

		currentCameraIndex++;
		availableCameras--;

		if (currentCameraIndex >= MAX_CAMERAS)
			currentCameraIndex = 0;
	}

	VertexArray *GXMRenderer::CreateVertexArray(const VertexInputDesc &desc, Buffer *vertexBuffer, Buffer *indexBuffer)
	{
		return new GXMVertexArray(desc, vertexBuffer, indexBuffer);
	}

	VertexArray *GXMRenderer::CreateVertexArray(const VertexInputDesc *descs, unsigned int descCount, const std::vector<Buffer*> &vertexBuffers, Buffer *indexBuffer)
	{
		return new GXMVertexArray(descs, descCount, vertexBuffers, indexBuffer);
	}

	Buffer *GXMRenderer::CreateVertexBuffer(const void *data, unsigned int size, BufferUsage usage)
	{
		return new GXMVertexBuffer(data, size, usage);
	}

	Buffer *GXMRenderer::CreateIndexBuffer(const void *data, unsigned int size, BufferUsage usage)
	{
		return new GXMIndexBuffer(data, size, usage);
	}

	Buffer *GXMRenderer::CreateUniformBuffer(const void *data, unsigned int size)
	{
		return new GXMUniformBuffer(data, size, BufferUsage::STATIC);
	}

	Buffer *GXMRenderer::CreateDrawIndirectBuffer(unsigned int size, const void *data)
	{
		return nullptr;
	}

	Buffer *GXMRenderer::CreateSSBO(unsigned int size, const void *data, unsigned int stride, BufferUsage usage)
	{
		return nullptr;
	}

	Framebuffer *GXMRenderer::CreateFramebuffer(const FramebufferDesc &desc)
	{
		return nullptr;
	}

	Shader *GXMRenderer::CreateShader(const std::string &vertexName, const std::string &fragmentName, const std::string &defines, const std::vector<VertexInputDesc> &descs, const BlendState &blendState)
	{
		unsigned int id = SID(vertexName + fragmentName + defines);

		// If the shader already exists return that one instead
		if (shaders.find(id) != shaders.end())
		{
			return shaders[id];
		}

		// Else create a new one
		Log::Print(LogLevel::LEVEL_INFO, "Created shadddddddddd\n");
		GXMShader *shader = new GXMShader(shaderPatcher, fileManager, vertexName, fragmentName, descs, blendState);		// No need to pass the defines because the shaders are compiled when building in the editor. When load the shader in the Vita it already has been compiled with the defines
		Log::Print(LogLevel::LEVEL_INFO, "Created shader\n");
		shaders[id] = shader;
		Log::Print(LogLevel::LEVEL_INFO, "Created shader 0\n");
		return shader;
	}

	Shader *GXMRenderer::CreateShader(const std::string &vertexName, const std::string &fragmentName, const std::vector<VertexInputDesc> &descs, const BlendState &blendState)
	{
		return CreateShader(vertexName, fragmentName, "", descs, blendState);
	}

	Shader *GXMRenderer::CreateShaderWithGeometry(const std::string &vertexPath, const std::string &geometryPath, const std::string &fragmentPath, const std::string &defines, const std::vector<VertexInputDesc> &descs)
	{
		return nullptr;
	}

	Shader *GXMRenderer::CreateShaderWithGeometry(const std::string &vertexPath, const std::string &geometryPath, const std::string &fragmentPath, const std::vector<VertexInputDesc>& descs)
	{
		return nullptr;
	}

	Shader *GXMRenderer::CreateComputeShader(const std::string &defines, const std::string &computePath)
	{
		return nullptr;
	}

	Shader *GXMRenderer::CreateComputeShader(const std::string &computePath)
	{
		return nullptr;
	}

	MaterialInstance *GXMRenderer::CreateMaterialInstance(ScriptManager &scriptManager, const std::string &matInstPath, const std::vector<VertexInputDesc> &inputDescs)
	{
		MaterialInstance *mat = Material::LoadMaterialInstance(this, matInstPath, scriptManager, inputDescs);
		materialInstances.push_back(mat);
		return mat;
	}

	MaterialInstance *GXMRenderer::CreateMaterialInstanceFromBaseMat(ScriptManager &scriptManager, const std::string &baseMatPath, const std::vector<VertexInputDesc> &inputDescs)
	{
		MaterialInstance *m = Material::LoadMaterialInstanceFromBaseMat(this, baseMatPath, scriptManager, inputDescs);
		materialInstances.push_back(m);

		return m;
	}

	void GXMRenderer::ReloadMaterial(Material *baseMaterial)
	{
	}

	Texture *GXMRenderer::CreateTexture2D(const std::string &path, const TextureParams &params, bool storeTextureData)
	{
		unsigned int id = SID(path);

		// If the texture already exists return that one instead
		if (textures.find(id) != textures.end())
		{
			Texture *tex = textures[id];
			tex->AddReference();
			return textures[id];
		}

		GXMTexture2D *tex = new GXMTexture2D();
		if (!tex->Load(fileManager, path, params, storeTextureData))
			return nullptr;

		tex->AddReference();
		textures[id] = tex;

		return tex;
	}

	Texture *GXMRenderer::CreateTexture3D(const std::string &path, const void *data, unsigned int width, unsigned int height, unsigned int depth, const TextureParams &params)
	{
		return nullptr;
	}

	Texture *GXMRenderer::CreateTextureCube(const std::vector<std::string> &faces, const TextureParams &params)
	{
		return nullptr;
	}

	Texture *GXMRenderer::CreateTextureCube(const std::string &path, const TextureParams & arams)
	{
		return nullptr;
	}

	Texture *GXMRenderer::CreateTexture2DFromData(unsigned int width, unsigned int height, const TextureParams &params, const void *data)
	{
		return nullptr;
	}

	Texture *GXMRenderer::CreateTexture3DFromData(unsigned int width, unsigned int height, unsigned int depth, const TextureParams &params, const void *data)
	{
		return nullptr;
	}

	void GXMRenderer::SetDefaultRenderTarget()
	{
		time += 1.0f / 60.0f;
		b = sin(time) * 0.5f + 0.5f;
		float myubo[] = { 0.0f, 0.0f, b };

		// wait for reads to the ubo to finish before we write to it
		fragmentIndex = (fragmentIndex + 1) % 2;
		sceGxmNotificationWait(&fragmentNotif[fragmentIndex]);

		ubo[fragmentIndex]->Update(myubo, sizeof(myubo), 0);

		const ColorSurface &cs = defaultFB->GetColorSurface(backBufferIndex);
		sceGxmBeginScene(context, 0, defaultFB->GetRTHandle(), nullptr, nullptr, cs.syncObj, &cs.surface, &defaultFB->GetDepthStencilSurface().surface);

		currentVertexProgram = clearShader->GetVertexProgram();
		currentFragmentProgram = clearShader->GetFragmentProgram();

		// Clear triangle
		sceGxmSetVertexProgram(context, currentVertexProgram);
		sceGxmSetFragmentProgram(context, currentFragmentProgram);

		void *colorBuffer;
		float clearColor[] = { 0.3f, 0.3f, 0.3f, 1.0f };
		sceGxmReserveFragmentDefaultUniformBuffer(context, &colorBuffer);
		sceGxmSetUniformDataF(colorBuffer, paramColorUniform, 0, 4, clearColor);

		sceGxmSetVertexStream(context, 0, clearVB->GetVerticesHandle());
		sceGxmDraw(context, SCE_GXM_PRIMITIVE_TRIANGLES, SCE_GXM_INDEX_FORMAT_U16, clearIB->GetIndicesHandle(), 3);
	}

	void GXMRenderer::SetRenderTarget(Framebuffer *rt)
	{
	}

	void GXMRenderer::SetRenderTargetAndClear(Framebuffer *rt)
	{
	}

	void GXMRenderer::EndRenderTarget(Framebuffer *rt)
	{
	}

	void GXMRenderer::EndDefaultRenderTarget()
	{
		fragmentNotif[fragmentIndex].value++;
		sceGxmEndScene(context, nullptr, &fragmentNotif[fragmentIndex]);
	}

	void GXMRenderer::ClearRenderTarget(Framebuffer *rt)
	{
	}

	void GXMRenderer::SetViewport(const Viewport &viewport)
	{
	}

	void GXMRenderer::Submit(const RenderQueue &renderQueue)
	{
		for (size_t i = 0; i < renderQueue.size(); i++)
		{
			Submit(renderQueue[i]);
		}
	}

	void GXMRenderer::Submit(const RenderItem &renderItem)
	{
		const ShaderPass &p = renderItem.matInstance->baseMaterial->GetShaderPass(renderItem.shaderPass);
		GXMShader *shader = static_cast<GXMShader*>(p.shader);

		SceGxmVertexProgram *newVertexProgram = shader->GetVertexProgram();
		SceGxmFragmentProgram *newFragmentProgram = shader->GetFragmentProgram();

		if (currentVertexProgram != newVertexProgram)
		{
			sceGxmSetVertexProgram(context, newVertexProgram);
			currentVertexProgram = newVertexProgram;
		}

		if (currentFragmentProgram != newFragmentProgram)
		{
			sceGxmSetFragmentProgram(context, newFragmentProgram);
			currentFragmentProgram = newFragmentProgram;
		}

		for (size_t i = 0; i < renderItem.matInstance->textures.size(); i++)
		{
			// Only set one texture for now
			if (i == 0)
			{
				Texture *t = renderItem.matInstance->textures[i];
				if (t)
				{
					GXMTexture2D *gt = static_cast<GXMTexture2D*>(renderItem.matInstance->textures[i]);
					sceGxmSetFragmentTexture(context, (unsigned int)i, &gt->GetGxmTexture());
				}				
			}
		}

		sceGxmSetFragmentUniformBuffer(context, 1, ubo[fragmentIndex]->GetUBO());
		
		const SceGxmProgramParameter *modelMatrixParam = shader->GetModelMatrixParam();
		if (modelMatrixParam)
		{
			glm::mat4 m = glm::mat4(1.0f);
			m = glm::rotate(m, glm::radians(time), glm::vec3(0.0f, 1.0f, 0.0f));

			void *buf;
			sceGxmReserveVertexDefaultUniformBuffer(context, &buf);
			sceGxmSetUniformDataF(buf, modelMatrixParam, 0, 16, &m[0].x);
		}
		

		SetDepthStencilState(p.depthStencilState);


		GXMVertexBuffer *vb = static_cast<GXMVertexBuffer*>(renderItem.mesh->vao->GetVertexBuffers()[0]);
		GXMIndexBuffer *ib = static_cast<GXMIndexBuffer*>(renderItem.mesh->vao->GetIndexBuffer());
		
		sceGxmSetVertexStream(context, 0, vb->GetVerticesHandle());

		if (renderItem.mesh->instanceCount > 0)
		{
			sceGxmDrawInstanced(context, SCE_GXM_PRIMITIVE_TRIANGLES, SCE_GXM_INDEX_FORMAT_U16, ib->GetIndicesHandle(), renderItem.mesh->indexCount * renderItem.mesh->instanceCount, renderItem.mesh->indexCount);
		}
		else
		{
			sceGxmDraw(context, SCE_GXM_PRIMITIVE_TRIANGLES, SCE_GXM_INDEX_FORMAT_U16, ib->GetIndicesHandle(), renderItem.mesh->indexCount);
		}
	}

	void GXMRenderer::SubmitIndirect(const RenderItem &renderItem, Buffer *indirectBuffer)
	{
	}

	void GXMRenderer::Dispatch(const DispatchItem &item)
	{
	}

	void GXMRenderer::Present()
	{
		const ColorSurface &cs = defaultFB->GetColorSurface(backBufferIndex);

		//Pad heartbeat to notify end of frame
		sceGxmPadHeartbeat(&cs.surface, cs.syncObj);

		// Queue the display swap for this frame
		displayQueueCallbackData displayData;
		displayData.addr = cs.addr;
		sceGxmDisplayQueueAddEntry(defaultFB->GetColorSurface(frontBufferIndex).syncObj, cs.syncObj, &displayData);

		frontBufferIndex = backBufferIndex;
		backBufferIndex = (backBufferIndex + 1) % DISPLAY_BUFFER_COUNT;
	}

	void GXMRenderer::AddResourceToSlot(unsigned int binding, Texture *texture, bool useStorage, unsigned int stages, bool separateMipViews)
	{
	}

	void GXMRenderer::AddResourceToSlot(unsigned int binding, Buffer *buffer, unsigned int stages)
	{
	}

	void GXMRenderer::SetupResources()
	{
	}

	void GXMRenderer::UpdateResourceOnSlot(unsigned int binding, Texture *texture, bool useStorage, bool separateMipViews)
	{

	}

	void GXMRenderer::PerformBarrier(const Barrier &barrier)
	{
	}

	void GXMRenderer::CopyImage(Texture *src, Texture *dst)
	{
	}

	void GXMRenderer::ClearImage(Texture *tex)
	{
	}

	void GXMRenderer::UpdateMaterialInstance(MaterialInstance *matInst)
	{
	}

	void GXMRenderer::SetDepthStencilState(const DepthStencilState &state)
	{
		bool changed = false;			// Using this variable so if depth enable is different we don't updated the state there because the depth write if would not pass because they would be equal
		if (depthStencilState.depthEnable != state.depthEnable)
		{
			changed = true;
			/*if (state.depthEnable)
				sceGxmDestroyContext
			else
				glDisable(GL_DEPTH_TEST);*/
		}
		if (depthStencilState.depthWrite != state.depthWrite)
		{
			changed = true;
			if (state.depthWrite)
				sceGxmSetFrontDepthWriteEnable(context, SCE_GXM_DEPTH_WRITE_ENABLED);
			else
				sceGxmSetFrontDepthWriteEnable(context, SCE_GXM_DEPTH_WRITE_DISABLED);
		}
		if (depthStencilState.depthFunc != state.depthFunc)
		{
			changed = true;
			//glDepthFunc(state.depthFunc);
		}

		if (changed)
			depthStencilState = state;
	}

	void GXMRenderer::SetRasterizerState(const RasterizerState &state)
	{
		bool changed = false;
		if (rasterizerState.cullFace != state.cullFace)
		{
			changed = true;
			/*glCullFace(state.cullFace);
			sceGxmSetCullMode(context, sce_gxm_cull)*/
		}
		if (rasterizerState.enableCulling != state.enableCulling)
		{
			changed = true;
			if (state.enableCulling)
				sceGxmSetCullMode(context, SCE_GXM_CULL_CW);
			else
				sceGxmSetCullMode(context, SCE_GXM_CULL_NONE);
		}

		if (changed)
		{
			rasterizerState = state;
			renderStats.cullingChanges++;
		}
	}

	void GXMRenderer::Dispose()
	{
		Log::Print(LogLevel::LEVEL_INFO, "Wainting Gxm finish\n");

		// wait until rendering is done
		sceGxmFinish(context);

		Log::Print(LogLevel::LEVEL_INFO, "Finished\n");

		// wait until display queue is finished before deallocating display buffers
		sceGxmDisplayQueueFinish();

		Log::Print(LogLevel::LEVEL_INFO, "Display queue finished\n");

		clearShader->Dispose(shaderPatcher);
		Log::Print(LogLevel::LEVEL_INFO, "dispose shader\n");
		delete clearShader;
		delete clearVB;
		Log::Print(LogLevel::LEVEL_INFO, "delete vb\n");
		delete clearIB;
		Log::Print(LogLevel::LEVEL_INFO, "delete ib\n");
		delete defaultFB;
		Log::Print(LogLevel::LEVEL_INFO, "delete fb\n");
		delete ubo[0];
		Log::Print(LogLevel::LEVEL_INFO, "delete ubo 0\n");
		delete ubo[1];
		Log::Print(LogLevel::LEVEL_INFO, "delete ubo 1\n");

		sceGxmShaderPatcherDestroy(shaderPatcher);
		gxmutils::gpuFragmentUsseUnmapFree(shaderPatcherBufferFragmentUsseUID);
		gxmutils::gpuVertexUsseUnmapFree(shaderPatcherBufferVertexUsseUID);
		gxmutils::graphicsFree(shaderPatcherBufferUID);

		// destroy the context
		sceGxmDestroyContext(context);
		gxmutils::gpuFragmentUsseUnmapFree(fragmentUsseRingBufferUID);
		gxmutils::graphicsFree(fragmentRingBufferUID);
		gxmutils::graphicsFree(vertexRingBufferUID);
		gxmutils::graphicsFree(vdmRingBufferUID);
		free(contextParams.hostMem);

		// terminate libgxm
		sceGxmTerminate();

		Log::Print(LogLevel::LEVEL_INFO, "Bye\n");
	}

	void GXMRenderer::DisplayQueueCallback(const void *callbackData)
	{
		SceDisplayFrameBuf display_fb = {};
		const displayQueueCallbackData *cb_data = static_cast<const displayQueueCallbackData*>(callbackData);

		display_fb.size = sizeof(display_fb);
		display_fb.base = cb_data->addr;
		display_fb.pitch = DISPLAY_STRIDE_IN_PIXELS;
		display_fb.pixelformat = DISPLAY_PIXEL_FORMAT;
		display_fb.width = DISPLAY_WIDTH;
		display_fb.height = DISPLAY_HEIGHT;

		sceDisplaySetFrameBuf(&display_fb, SCE_DISPLAY_SETBUF_NEXTFRAME);

		sceDisplayWaitVblankStart();
	}

	void *GXMRenderer::ShaderPatcherHostAllocCb(void *user_data, unsigned int size)
	{
		return malloc(size);
	}

	void GXMRenderer::ShaderPatcherHostFreeCb(void *user_data, void *mem)
	{
		return free(mem);
	}
}
