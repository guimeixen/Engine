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
#include "Graphics/MeshDefaults.h"
#include "Program/Input.h"
#include "psp2/ctrl.h"

#include "Data/Shaders/GXM/include/common.cgh"

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
		backBufferIndex = 0;
		frontBufferIndex = 0;
		numVertexTextureBindings = 0;
		numFragmentTextureBindings = 0;
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
		depthBiasFactor = 3;
		depthBiasUnits = 2;
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

		clearColorSurfaceData[0] = gxmutils::graphicsAlloc(SCE_KERNEL_MEMBLOCK_TYPE_USER_CDRAM_RW, SCE_GXM_MEMORY_ATTRIB_RW, ALIGN(4 * DISPLAY_STRIDE_IN_PIXELS * height, 1 * 1024 * 1024), &clearColorDataUIDs[0]);
		memset(clearColorSurfaceData[0], 0, DISPLAY_STRIDE_IN_PIXELS * height);
		clearColorSurfaceData[1] = gxmutils::graphicsAlloc(SCE_KERNEL_MEMBLOCK_TYPE_USER_CDRAM_RW, SCE_GXM_MEMORY_ATTRIB_RW, ALIGN(4 * DISPLAY_STRIDE_IN_PIXELS * height, 1 * 1024 * 1024), &clearColorDataUIDs[1]);
		memset(clearColorSurfaceData[1], 0, DISPLAY_STRIDE_IN_PIXELS * height);

		sceGxmColorSurfaceInit(&clearColorSurfaces[0], DISPLAY_COLOR_FORMAT, SCE_GXM_COLOR_SURFACE_LINEAR, SCE_GXM_COLOR_SURFACE_SCALE_NONE, SCE_GXM_OUTPUT_REGISTER_SIZE_32BIT, width, height, DISPLAY_STRIDE_IN_PIXELS, clearColorSurfaceData[0]);
		sceGxmSyncObjectCreate(&clearSyncObjs[0]);
		sceGxmColorSurfaceInit(&clearColorSurfaces[1], DISPLAY_COLOR_FORMAT, SCE_GXM_COLOR_SURFACE_LINEAR, SCE_GXM_COLOR_SURFACE_SCALE_NONE, SCE_GXM_OUTPUT_REGISTER_SIZE_32BIT, width, height, DISPLAY_STRIDE_IN_PIXELS, clearColorSurfaceData[1]);
		sceGxmSyncObjectCreate(&clearSyncObjs[1]);

		unsigned int depthStencilWidth = ALIGN(width, SCE_GXM_TILE_SIZEX);
		unsigned int depthStencilHeight = ALIGN(height, SCE_GXM_TILE_SIZEY);
		unsigned int depthStencilSamples = depthStencilWidth * depthStencilHeight;

		clearDepthSurfaceData = gxmutils::graphicsAlloc(SCE_KERNEL_MEMBLOCK_TYPE_USER_RW_UNCACHE, SCE_GXM_MEMORY_ATTRIB_RW, 4 * depthStencilSamples, &clearDepthDataUID);
		sceGxmDepthStencilSurfaceInit(&clearDepthStencilSurface, SCE_GXM_DEPTH_STENCIL_FORMAT_S8D24, SCE_GXM_DEPTH_STENCIL_SURFACE_TILED, depthStencilWidth, clearDepthSurfaceData, nullptr);

		SceGxmRenderTargetParams rtParams = {};
		rtParams.flags = 0;
		rtParams.width = (uint16_t)width;
		rtParams.height = (uint16_t)height;
		rtParams.scenesPerFrame = 1;
		rtParams.multisampleMode = SCE_GXM_MULTISAMPLE_NONE;
		rtParams.multisampleLocations = 0;
		rtParams.driverMemBlock = -1;

		sceGxmCreateRenderTarget(&rtParams, &defaultRenderTarget);

		/*FramebufferDesc desc = {};
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

		defaultFB = new GXMFramebuffer(desc);*/

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
		b.enableColorWriting = true;
		b.enableBlending = false;
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

		for (size_t i = 0; i < MAX_CAMERAS; i++)
		{
			cameraUBOs[i] = new GXMUniformBuffer(nullptr, sizeof(CameraUBOSimple), BufferUsage::STATIC);
		}

		sceGxmSetVertexUniformBuffer(context, CAMERA_UBO_SLOT, cameraUBOs[currentCameraIndex]->GetData());

		sceGxmSetCullMode(context, SCE_GXM_CULL_CW);

		depthStencilState.depthEnable = true;
		//depthStencilState.depthFunc = ;
		depthStencilState.depthWrite = true;
		rasterizerState.enableCulling = true;
		rasterizerState.cullFace = SCE_GXM_CULL_CW;

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

		/*CameraUBO ubo = {};
		ubo.proj = proj;
		ubo.view = view;
		ubo.projView = proj * view;	
		ubo.invView = glm::inverse(ubo.view);
		ubo.invProj = glm::inverse(ubo.proj);
		ubo.clipPlane = clipPlane;
		ubo.camPos = glm::vec4(camera->GetPosition(), 0.0f);
		ubo.nearFarPlane = glm::vec2(camera->GetNearPlane(), camera->GetFarPlane());*/

		CameraUBOSimple ubo = {};
		ubo.projView = proj * view;
		ubo.camPos = glm::vec4(camera->GetPosition(), 0.0f);

		// If we don't have more cameras available, we need to wait.
		/*if (availableCameras < 1)
		{

		}*/

		cameraUBOs[currentCameraIndex]->Update(&ubo, sizeof(ubo), 0);

		sceGxmSetVertexUniformBuffer(context, CAMERA_UBO_SLOT, cameraUBOs[currentCameraIndex]->GetData());
		sceGxmSetFragmentUniformBuffer(context, CAMERA_UBO_SLOT, cameraUBOs[currentCameraIndex]->GetData());

		currentCameraIndex++;
		//availableCameras--;

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
		return new GXMUniformBuffer(data, size, BufferUsage::DYNAMIC);
	}

	Framebuffer *GXMRenderer::CreateFramebuffer(const FramebufferDesc &desc)
	{
		return new GXMFramebuffer(desc);
	}

	ShaderProgram* GXMRenderer::CreateShader(const std::string &vertexName, const std::string &fragmentName, const std::string &defines, const std::vector<VertexInputDesc> &descs, const BlendState &blendState)
	{
		unsigned int id = SID(vertexName + fragmentName + defines);

		// If the shader already exists return that one instead
		if (shaderPrograms.find(id) != shaderPrograms.end())
		{
			return shaderPrograms[id];
		}

		// Else create a new one
		GXMShader *shader = new GXMShader(shaderPatcher, fileManager, vertexName, fragmentName, descs, blendState);		// No need to pass the defines because the shaders are compiled when building in the editor. When load the shader in the Vita it already has been compiled with the defines
		shaderPrograms[id] = shader;
		return shader;
	}

	ShaderProgram* GXMRenderer::CreateShader(const std::string &vertexName, const std::string &fragmentName, const std::vector<VertexInputDesc> &descs, const BlendState &blendState)
	{
		return CreateShader(vertexName, fragmentName, "", descs, blendState);
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

	void GXMRenderer::SetDefaultRenderTarget()
	{
		/*time += 1.0f / 60.0f;
		b = sin(time) * 0.5f + 0.5f;
		float myubo[] = { 0.0f, 0.0f, b };

		// wait for reads to the ubo to finish before we write to it
		fragmentIndex = (fragmentIndex + 1) % 2;
		sceGxmNotificationWait(&fragmentNotif[fragmentIndex]);

		ubo[fragmentIndex]->Update(myubo, sizeof(myubo), 0);*/

		sceGxmBeginScene(context, 0, defaultRenderTarget, nullptr, nullptr, clearSyncObjs[backBufferIndex], &clearColorSurfaces[backBufferIndex], &clearDepthStencilSurface);

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

	void GXMRenderer::SetRenderTargetAndClear(Framebuffer *rt)
	{
		GXMFramebuffer *fb = static_cast<GXMFramebuffer*>(rt);

		if (fb->GetNumColorTextures() > 0)
			sceGxmBeginScene(context, 0, fb->GetRenderTarget(), nullptr, nullptr, nullptr, &fb->GetColorSurface(0).surface, &fb->GetDepthStencilSurface().surface);
		else
			sceGxmBeginScene(context, 0, fb->GetRenderTarget(), nullptr, nullptr, nullptr, nullptr, &fb->GetDepthStencilSurface().surface);
		
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

		//if (fb->GetNumColorTextures() == 0)
		//	sceGxmSetFrontDepthBias(context, (int)depthBiasFactor, (int)depthBiasUnits);
	}

	void GXMRenderer::EndRenderTarget(Framebuffer *rt)
	{
		sceGxmEndScene(context, nullptr, nullptr);
		//sceGxmSetFrontDepthBias(context, 0, 0);
	}

	void GXMRenderer::EndDefaultRenderTarget()
	{
		//fragmentNotif[fragmentIndex].value++;
		//sceGxmEndScene(context, nullptr, &fragmentNotif[fragmentIndex]);
		sceGxmEndScene(context, nullptr, nullptr);
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

		//Log::Print(LogLevel::LEVEL_INFO, "4.1\n");
		
		if (currentVertexProgram != newVertexProgram)
		{
			sceGxmSetVertexProgram(context, newVertexProgram);
			currentVertexProgram = newVertexProgram;
		}
		
		//Log::Print(LogLevel::LEVEL_INFO, "4.2\n");

		if (currentFragmentProgram != newFragmentProgram)
		{
			sceGxmSetFragmentProgram(context, newFragmentProgram);
			currentFragmentProgram = newFragmentProgram;
		}

		//Log::Print(LogLevel::LEVEL_INFO, "4.3\n");

		for (size_t i = 0; i < renderItem.matInstance->textures.size(); i++)
		{
			// Only set one texture for now
			if (i == 0)
			{
				Texture *t = renderItem.matInstance->textures[i];
				if (t)
				{
					GXMTexture2D *gt = static_cast<GXMTexture2D*>(t);
					sceGxmSetFragmentTexture(context, (unsigned int)i + numFragmentTextureBindings, &gt->GetGxmTexture());
				}				
			}
		}

		//Log::Print(LogLevel::LEVEL_INFO, "4.4\n");


		//sceGxmSetFragmentUniformBuffer(context, 1, ubo[fragmentIndex]->GetData());
		
		const SceGxmProgramParameter *modelMatrixParam = shader->GetModelMatrixParam();
		if (modelMatrixParam && renderItem.transform)
		{
			const glm::mat4 &m = *renderItem.transform;

			void *buf;
			sceGxmReserveVertexDefaultUniformBuffer(context, &buf);
			sceGxmSetUniformDataF(buf, modelMatrixParam, 0, 16, &m[0].x);
		}
		
		//Log::Print(LogLevel::LEVEL_INFO, "4.5\n");

		SetDepthStencilState(p.depthStencilState);

		//Log::Print(LogLevel::LEVEL_INFO, "4.6\n");

		GXMVertexBuffer *vb = static_cast<GXMVertexBuffer*>(renderItem.mesh->vao->GetVertexBuffers()[0]);
		GXMIndexBuffer *ib = static_cast<GXMIndexBuffer*>(renderItem.mesh->vao->GetIndexBuffer());
		
		sceGxmSetVertexStream(context, 0, vb->GetVerticesHandle());

		//Log::Print(LogLevel::LEVEL_INFO, "4.7\n");

		if (renderItem.mesh->instanceCount > 0)
		{
			sceGxmDrawInstanced(context, SCE_GXM_PRIMITIVE_TRIANGLES, SCE_GXM_INDEX_FORMAT_U16, ib->GetIndicesHandle(), renderItem.mesh->indexCount * renderItem.mesh->instanceCount, renderItem.mesh->indexCount);
		}
		else
		{
			sceGxmDraw(context, SCE_GXM_PRIMITIVE_TRIANGLES, SCE_GXM_INDEX_FORMAT_U16, ib->GetIndicesHandle(), renderItem.mesh->indexCount);
		}

		//Log::Print(LogLevel::LEVEL_INFO, "4.8\n");
	}

	void GXMRenderer::Present()
	{
		if (Input::IsVitaButtonDown(SCE_CTRL_CIRCLE))
		{
			depthBiasFactor++;
		}
		if (Input::IsVitaButtonDown(SCE_CTRL_SQUARE))
		{
			depthBiasFactor--;
		}
		if (Input::IsVitaButtonDown(SCE_CTRL_CROSS))
		{
			depthBiasUnits--;
		}
		if (Input::IsVitaButtonDown(SCE_CTRL_TRIANGLE))
		{
			depthBiasUnits++;
		}

		//Pad heartbeat to notify end of frame
		sceGxmPadHeartbeat(&clearColorSurfaces[backBufferIndex], clearSyncObjs[backBufferIndex]);

		// Queue the display swap for this frame
		displayQueueCallbackData displayData;
		displayData.addr = clearColorSurfaceData[backBufferIndex];
		sceGxmDisplayQueueAddEntry(clearSyncObjs[frontBufferIndex], clearSyncObjs[backBufferIndex], &displayData);

		frontBufferIndex = backBufferIndex;
		backBufferIndex = (backBufferIndex + 1) % DISPLAY_BUFFER_COUNT;
	}

	void GXMRenderer::AddTextureResourceToSlot(unsigned int binding, Texture *texture, bool useStorage, unsigned int stages, TextureInternalFormat viewFormat, bool separateMipViews)
	{
		if (!texture)
			return;

		if (texture->GetType() == TextureType::TEXTURE2D)
		{
			GXMTexture2D *tex = static_cast<GXMTexture2D*>(texture);
			if (stages & VERTEX)
			{
				sceGxmSetVertexTexture(context, binding, &tex->GetGxmTexture());
				numVertexTextureBindings++;
				Log::Print(LogLevel::LEVEL_INFO, "Added texture in binding %u to vertex\n", binding);
			}
			if (stages & FRAGMENT)
			{
				sceGxmSetFragmentTexture(context, binding, &tex->GetGxmTexture());
				numFragmentTextureBindings++;
				Log::Print(LogLevel::LEVEL_INFO, "Added texture in binding %u to fragment\n", binding);
			}
		}
	}

	void GXMRenderer::AddBufferResourceToSlot(unsigned int binding, Buffer *buffer, unsigned int stages)
	{
		if (!buffer)
			return;

		if (buffer->GetType() == BufferType::UniformBuffer)
		{
			GXMUniformBuffer *ubo = static_cast<GXMUniformBuffer*>(buffer);
			if (stages & VERTEX)
			{
				sceGxmSetVertexUniformBuffer(context, binding, ubo->GetData());
				Log::Print(LogLevel::LEVEL_INFO, "Added to vertex\n");
			}
			if (stages & FRAGMENT)
			{
				sceGxmSetFragmentUniformBuffer(context, binding, ubo->GetData());
				Log::Print(LogLevel::LEVEL_INFO, "Added to fragment\n");
			}
		}
	}

	void GXMRenderer::SetupResources()
	{
	}

	void GXMRenderer::UpdateTextureResourceOnSlot(unsigned int binding, Texture *texture, bool useStorage, bool separateMipViews)
	{

	}

	void GXMRenderer::PerformBarrier(const Barrier &barrier)
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
			sceGxmSetCullMode(context, (SceGxmCullMode)state.cullFace);
		}
		if (rasterizerState.enableCulling != state.enableCulling)
		{
			changed = true;
			if (state.enableCulling)
				sceGxmSetCullMode(context, SCE_GXM_CULL_CW);			// Cull back faces which are CW, front faces are CCW
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
		delete clearShader;
		delete clearVB;
		delete clearIB;
		delete ubo[0];
		delete ubo[1];

		gxmutils::graphicsFree(clearDepthDataUID);
		for (unsigned int i = 0; i < DISPLAY_BUFFER_COUNT; ++i)
		{
			// clear the buffer then deallocate
			memset(clearColorSurfaceData[i], 0, height * DISPLAY_STRIDE_IN_PIXELS * 4);
			gxmutils::graphicsFree(clearColorDataUIDs[i]);

			// destroy the sync object
			sceGxmSyncObjectDestroy(clearSyncObjs[i]);
		}

		sceGxmDestroyRenderTarget(defaultRenderTarget);

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
