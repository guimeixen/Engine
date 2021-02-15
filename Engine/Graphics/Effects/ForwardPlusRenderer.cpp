#include "ForwardPlusRenderer.h"

#include "Game/Game.h"
#include "Graphics/VertexArray.h"
#include "Program/StringID.h"
#include "Program/Random.h"
#include "Graphics/Lights.h"

#include "include/glm/gtc/matrix_transform.hpp"

#include "Data/Shaders/bindings.glsl"

#include <iostream>

namespace Engine
{
	ForwardPlusRenderer::ForwardPlusRenderer()
	{
		renderingPathType = RenderingPathType::FORWARD_PLUS;

		frustumsSSBO = nullptr;
		lightListSSBO = nullptr;
		opaqueLightIndexCounterSSBO = nullptr;
		opaqueLightIndexListSSBO = nullptr;
		lightGrid = nullptr;
	}

	void ForwardPlusRenderer::Init(Game *game)
	{
		RenderingPath::Init(game);

		renderer->AddGlobalDefine("FORWARD_PLUS");

		depthPrepassQueueID = SID("depthPrepass");

		unsigned int numTiles = (unsigned int)std::ceil((float)width / LIGHT_TILE_SIZE) * (unsigned int)std::ceil((float)height / LIGHT_TILE_SIZE);			// Also the number of frustums
		frustumsPassNumWorkGroupsX = (unsigned int)std::ceil((float)width / LIGHT_TILE_SIZE / 16);	// 16 -> number of threads in the work group
		frustumsPassNumWorkGroupsY = (unsigned int)std::ceil((float)height / LIGHT_TILE_SIZE / 16);

		numFrustums = glm::uvec2((unsigned int)std::ceil((float)width / LIGHT_TILE_SIZE), (unsigned int)std::ceil((float)height / LIGHT_TILE_SIZE));

		unsigned int avgLightsPerTile = 64;

		frustumsSSBO = renderer->CreateSSBO(numTiles * sizeof(ShaderFrustum), nullptr, sizeof(ShaderFrustum), BufferUsage::STATIC);		// TODO: Improve usage enum, should have something like CPU_UPDATED,...
		lightListSSBO = renderer->CreateSSBO(MAX_LIGHTS * sizeof(LightShader), nullptr, sizeof(LightShader), BufferUsage::DYNAMIC);
		opaqueLightIndexListSSBO = renderer->CreateSSBO(avgLightsPerTile * numTiles * sizeof(unsigned int), nullptr, sizeof(unsigned int), BufferUsage::STATIC);
		opaqueLightIndexCounterSSBO = renderer->CreateSSBO(sizeof(unsigned int), nullptr, 0, BufferUsage::STATIC);

		frustumsSSBO->AddReference();
		lightListSSBO->AddReference();
		opaqueLightIndexListSSBO->AddReference();
		opaqueLightIndexCounterSSBO->AddReference();

		TextureParams params = {};
		params.filter = TextureFilter::NEAREST;
		params.format = TextureFormat::RG;
		params.internalFormat = TextureInternalFormat::RG32UI;
		params.type = TextureDataType::UNSIGNED_INT;
		params.usedAsStorageInCompute = true;
		params.usedAsStorageInGraphics = true;
		params.wrap = TextureWrap::CLAMP_TO_EDGE;

		lightGrid = renderer->CreateTexture2DFromData((unsigned int)std::ceil((float)width / LIGHT_TILE_SIZE), (unsigned int)std::ceil((float)height / LIGHT_TILE_SIZE), params, nullptr);

		SetupComputeFrustumsPass();
		SetupDepthPrepass();
		SetupLightCullingPass();
		SetupHDRPass();
		SetupPostProcessPass();

		//frameGraph.SetBackbufferSource("fxaa");		// Post process pass writes to backbuffer
		frameGraph.SetBackbufferSource("postProcessPass");
		frameGraph.Bake(renderer);
		frameGraph.ExportGraphVizFile();

		renderer->AddTextureResourceToSlot(CSM_TEXTURE, frameGraph.GetPass("csm").GetFramebuffer()->GetDepthTexture(), false, PipelineStage::FRAGMENT);
		renderer->AddTextureResourceToSlot(LIGHT_GRID_TEXTURE, lightGrid, true, PipelineStage::COMPUTE | PipelineStage::FRAGMENT);

		renderer->AddBufferResourceToSlot(FRUSTUMS_SSBO, frustumsSSBO, PipelineStage::COMPUTE);
		renderer->AddBufferResourceToSlot(LIGHT_LIST_SSBO, lightListSSBO, PipelineStage::COMPUTE | PipelineStage::FRAGMENT);
		renderer->AddBufferResourceToSlot(OPAQUE_LIGHT_INDEX_LIST_SSBO, opaqueLightIndexListSSBO, PipelineStage::COMPUTE | PipelineStage::FRAGMENT);
		renderer->AddBufferResourceToSlot(OPAQUE_LIGHT_INDEX_COUNTER_SSBO, opaqueLightIndexCounterSSBO, PipelineStage::COMPUTE);
		
		renderer->SetupResources();
		
		frustumsMat = renderer->CreateMaterialInstanceFromBaseMat(game->GetScriptManager(), "Data/Resources/Materials/forward_plus/frustums_mat.lua", {});
		lightCullingMat = renderer->CreateMaterialInstanceFromBaseMat(game->GetScriptManager(), "Data/Resources/Materials/forward_plus/light_culling_mat.lua", {});
	}

	void ForwardPlusRenderer::Dispose()
	{
		RenderingPath::Dispose();

		if (frustumsSSBO)
			frustumsSSBO->RemoveReference();
		if (lightListSSBO)
			lightListSSBO->RemoveReference();
		if (opaqueLightIndexCounterSSBO)
			opaqueLightIndexCounterSSBO->RemoveReference();
		if (opaqueLightIndexListSSBO)
			opaqueLightIndexListSSBO->RemoveReference();
		if (lightGrid)
			lightGrid->RemoveReference();

		frustumsSSBO = nullptr;
		lightListSSBO = nullptr;
		opaqueLightIndexListSSBO = nullptr;
		opaqueLightIndexCounterSSBO = nullptr;
		lightGrid = nullptr;
	}

	void ForwardPlusRenderer::Resize(unsigned int width, unsigned int height)
	{
		RenderingPath::Resize(width, height);

		frameGraph.GetPass("depthPrepass").Resize(width, height);
		frameGraph.Bake(renderer);

		renderer->UpdateMaterialInstance(postProcMatInstance);
		renderer->UpdateMaterialInstance(brightPassMatInstance);
		renderer->UpdateMaterialInstance(downsample4MatInstance);
		renderer->UpdateMaterialInstance(downsample8MatInstance);
		renderer->UpdateMaterialInstance(downsample16MatInstance);
		renderer->UpdateMaterialInstance(upsample8MatInstance);
		renderer->UpdateMaterialInstance(upsample4MatInstance);
		//renderer->UpdateMaterialInstance(fxaaMat);

		//numTiles = width / LIGHT_TILE_SIZE * height / LIGHT_TILE_SIZE;			// Also the number of frustums
		frustumsPassNumWorkGroupsX = (unsigned int)std::ceil((float)width / LIGHT_TILE_SIZE / 16);	// 16 -> number of threads in the work group
		frustumsPassNumWorkGroupsY = (unsigned int)std::ceil((float)height / LIGHT_TILE_SIZE / 16);

		numFrustums = glm::uvec2((unsigned int)std::ceil((float)width / LIGHT_TILE_SIZE), (unsigned int)std::ceil((float)height / LIGHT_TILE_SIZE));

		TextureParams params = {};
		params.filter = TextureFilter::NEAREST;
		params.format = TextureFormat::RG;
		params.internalFormat = TextureInternalFormat::RG32UI;
		params.type = TextureDataType::UNSIGNED_INT;
		params.usedAsStorageInCompute = true;
		params.usedAsStorageInGraphics = true;
		params.wrap = TextureWrap::CLAMP_TO_EDGE;

		lightGrid->RemoveReference();
		renderer->RemoveTexture(lightGrid);

		lightGrid = renderer->CreateTexture2DFromData((unsigned int)std::ceil((float)width / LIGHT_TILE_SIZE), (unsigned int)std::ceil((float)height / LIGHT_TILE_SIZE), params, nullptr);
		renderer->UpdateTextureResourceOnSlot(LIGHT_GRID_TEXTURE, lightGrid, true);
	}

	void ForwardPlusRenderer::Render()
	{
		RenderingPath::Render();

		const LightManager &lightManager = game->GetLightManager();
		const std::vector<LightShader> &lights = lightManager.GetLightsShaderReady();
		//curLightCount = (unsigned int)lights.size();
		curLightCount = lightManager.GetEnabledPointLightsCount();

		lightListSSBO->Update(lights.data(), (unsigned int)lights.size() * sizeof(LightShader), 0);

		unsigned int queueIDs[] = { csmQueueID, csmQueueID, csmQueueID, voxelizationQueueID, opaqueQueueID, transparentQueueID, uiQueueID, depthPrepassQueueID };

		const glm::vec3 &camPos = mainCamera->GetPosition();
		float voxelGridSize = vctgi.GetVoxelGridSize();
		float voxelGridHalfSize = voxelGridSize * 0.5f;

		Frustum ortho;
		ortho.UpdateProjection(camPos.x - voxelGridHalfSize, camPos.x + voxelGridHalfSize, camPos.y - voxelGridHalfSize, camPos.y + voxelGridHalfSize, camPos.z - voxelGridHalfSize, camPos.z + voxelGridHalfSize);
		ortho.Update(camPos, camPos + mainCamera->GetFront(), mainCamera->GetUp());

		// TODO: Perform culling only once per frustum and not multiple times
		Frustum frustums[7];
		frustums[0] = csmInfo.cameras[0].GetFrustum();
		frustums[1] = csmInfo.cameras[1].GetFrustum();
		frustums[2] = csmInfo.cameras[2].GetFrustum();
		frustums[3] = ortho;
		frustums[4] = mainCamera->GetFrustum();
		frustums[5] = mainCamera->GetFrustum();
		frustums[6] = uiCamera.GetFrustum();

		std::vector<VisibilityIndices> visibility = renderer->Cull(7, queueIDs, frustums);

		renderer->CopyVisibilityToQueue(visibility, 4, 7);		// Copy the visibility to the depth prepass queue so we don't have to perform culling again
		renderer->CreateRenderQueues(8, queueIDs, visibility, renderQueues);
		//renderer->CreateRenderQueues(7, queueIDs, frustums, renderQueues);

		vctgi.EndFrame();
		frameGraph.Execute(renderer);

		frameData.previousFrameView = mainCamera->GetViewMatrix();

		volumetricClouds.EndFrame();
	}

	void ForwardPlusRenderer::SetupComputeFrustumsPass()
	{
		Pass &pass = frameGraph.AddPass("frustumsPass");
		pass.SetIsCompute(true);
		pass.AddBufferOutput("frustumsBuffer", frustumsSSBO);

		pass.OnBarriers([this]()
		{
			// Make sure the buffer has been read
			BarrierBuffer bb = {};
			bb.buffer = frustumsSSBO;
			bb.readToWrite = true;

			Barrier b = {};
			b.buffers.push_back(bb);
			b.srcStage = PipelineStage::COMPUTE;
			b.dstStage = PipelineStage::COMPUTE;

			renderer->PerformBarrier(b);
		});
		pass.OnExecute([this]()
		{
			DispatchItem item = {};
			item.matInstance = frustumsMat;
			item.numGroupsX = frustumsPassNumWorkGroupsX;
			item.numGroupsY = frustumsPassNumWorkGroupsY;
			item.numGroupsZ = 1;
			item.shaderPass = 0;
			item.materialData = &numFrustums.x;
			item.materialDataSize = sizeof(glm::uvec2);

			renderer->SetCamera(mainCamera);
			renderer->Dispatch(item);
		});
	}

	void ForwardPlusRenderer::SetupDepthPrepass()
	{
		Pass &p = frameGraph.AddPass("depthPrepass");

		AttachmentInfo info = {};
		info.width = width;
		info.height = height;
		info.params = { TextureWrap::CLAMP_TO_EDGE, TextureFilter::LINEAR, TextureFormat::DEPTH_COMPONENT, TextureInternalFormat::DEPTH_COMPONENT24, TextureDataType::FLOAT, false, false };

		p.AddDepthOutput("depthPrepassOutput", info);

		p.OnSetup([this](const Pass *thisPass)
		{
			depthPrepassFB = thisPass->GetFramebuffer();
			lightCullingMat->textures[0] = depthPrepassFB->GetDepthTexture();
			renderer->UpdateMaterialInstance(lightCullingMat);
		});

		p.OnResized([this](const Pass *thisPass)
		{
			lightCullingMat->textures[0] = depthPrepassFB->GetDepthTexture();
			renderer->UpdateMaterialInstance(lightCullingMat);
		});

		p.OnBarriers([this]()
		{
			// Make sure reads to the depth attachment from light culling pass is done
			/*BarrierImage bi = {};
			bi.image = depthPrepassFB->GetDepthTexture();
			bi.readToWrite = true;
			bi.baseMip = 0;
			bi.numMips = 1;

			Barrier b = {};
			b.images.push_back(bi);
			b.srcStage = PipelineStage::COMPUTE;
			b.dstStage = PipelineStage::DEPTH_STENCIL_WRITE;

			renderer->PerformBarrier(b);*/
		});

		p.OnExecute([this]()
		{
			renderer->Submit(renderQueues[7]);
			//std::cout << renderQueues[7].size() << '\n';
		});
	}

	void ForwardPlusRenderer::SetupLightCullingPass()
	{
		Pass &p = frameGraph.AddPass("lightCullingPass");
		p.SetIsCompute(true);
		p.AddDepthInput("depthPrepassOutput");
		p.AddImageOutput("lightGrid", lightGrid);
		p.AddBufferInput("frustumsBuffer", frustumsSSBO);
		//p.AddImageOutput("");

		p.OnBarriers([this]()
		{
			// Wait for the writes to the depth attachment from the depth prepass to be completed
			/*BarrierImage bi = {};
			bi.image = depthPrepassFB->GetDepthTexture();
			bi.readToWrite = false;
			bi.baseMip = 0;
			bi.numMips = 1;
			*/
			// Wait for writes to the frustums buffer to be completed
			BarrierBuffer bb = {};
			bb.buffer = frustumsSSBO;
			bb.readToWrite = false;

			Barrier b = {};
			//b.images.push_back(bi);
			b.buffers.push_back(bb);
			//b.srcStage = PipelineStage::DEPTH_STENCIL_WRITE | PipelineStage::COMPUTE;
			b.srcStage = PipelineStage::COMPUTE;
			b.dstStage = PipelineStage::COMPUTE;

			renderer->PerformBarrier(b);
		});

		p.OnExecute([this]()
		{
			DispatchItem item = {};
			item.matInstance = lightCullingMat;
			item.numGroupsX = (unsigned int)std::ceil((float)width / LIGHT_TILE_SIZE);
			item.numGroupsY = (unsigned int)std::ceil((float)height / LIGHT_TILE_SIZE);
			item.numGroupsZ = 1;
			item.materialData = &curLightCount;
			item.materialDataSize = sizeof(unsigned int);

			renderer->BindImage(LIGHT_GRID_TEXTURE, 0, lightGrid, ImageAccess::WRITE_ONLY);
			renderer->Dispatch(item);
		});
	}

	void ForwardPlusRenderer::SetupHDRPass()
	{
		Pass &hdrPass = frameGraph.AddPass("base");

		AttachmentInfo colorAttachment = {};
		colorAttachment.params = { TextureWrap::CLAMP_TO_EDGE, TextureFilter::LINEAR, TextureFormat::RGBA, TextureInternalFormat::RGBA16F, TextureDataType::FLOAT, false, false };
		colorAttachment.width = width;
		colorAttachment.height = height;
		colorAttachment.initialState = InitialState::CLEAR;
		/*AttachmentInfo normalAttach = {};
		normalAttach.params = { TextureWrap::CLAMP_TO_EDGE, TextureFilter::NEAREST, TextureFormat::RGB, TextureInternalFormat::RGB16F, TextureDataType::FLOAT, false };
		normalAttach.width = width;
		normalAttach.height = height;
		normalAttach.initialState = InitialState::CLEAR;*/
		AttachmentInfo depthAttachment = {};
		depthAttachment.params = { TextureWrap::CLAMP_TO_EDGE, TextureFilter::NEAREST, TextureFormat::DEPTH_COMPONENT, TextureInternalFormat::DEPTH_COMPONENT24, TextureDataType::FLOAT, false, false };
		depthAttachment.width = width;
		depthAttachment.height = height;
		depthAttachment.initialState = InitialState::CLEAR;

		hdrPass.AddImageInput("terrainImg");
		hdrPass.AddImageInput("voxelTextureMipmapped", true);
		hdrPass.AddBufferInput("voxelsIndirectBuffer", vctgi.GetIndirectBuffer());
		hdrPass.AddBufferInput("voxelPositionsBuffer", vctgi.GetVoxelPositionsBuffer());
		hdrPass.AddDepthInput("shadowMap");
		hdrPass.AddTextureInput("reflectionTex");
		hdrPass.AddTextureInput("refractionTex");
		hdrPass.AddDepthInput("refractionDepth");
		hdrPass.AddImageInput("lightGrid");
		//hdrPass.AddBufferInput("")
		hdrPass.AddTextureOutput("color", colorAttachment);
		hdrPass.AddDepthOutput("depth", depthAttachment);	

		hdrPass.OnSetup([this](const Pass *thisPass)
		{
			hdrFB = thisPass->GetFramebuffer();

			tod.Init(game);

			//GenerateLightning();
			vctgi.CreateMat(game->GetScriptManager());
		});

		hdrPass.OnBarriers([this]()
		{
			Barrier b = {};

			BarrierImage bi = {};
			bi.image = lightGrid;
			bi.readToWrite = false;
			bi.baseMip = 0;
			bi.numMips = 1;

			// Wait for the light index list to be written
			BarrierBuffer bb = {};
			bb.buffer = opaqueLightIndexListSSBO;
			bb.readToWrite = false;

			BarrierBuffer bb1 = {};
			bb1.buffer = vctgi.GetIndirectBuffer();
			bb1.readToWrite = false;

			BarrierBuffer bb2 = {};
			bb2.buffer = vctgi.GetVoxelPositionsBuffer();
			bb2.readToWrite = false;

			b.images.push_back(bi);
			b.buffers.push_back(bb);

			b.srcStage = PipelineStage::COMPUTE;

			if (debugSettings.enableVoxelVis)
			{
				b.buffers.push_back(bb1);
				b.buffers.push_back(bb2);
				b.dstStage = PipelineStage::FRAGMENT | PipelineStage::INDIRECT | PipelineStage::VERTEX;
			}
			else
			{
				b.dstStage = PipelineStage::FRAGMENT;
			}

			renderer->PerformBarrier(b);
		});

		hdrPass.OnExecute([this]() {PerformHDRPass(); });
	}

	void ForwardPlusRenderer::SetupPostProcessPass()
	{
		Pass &postPass = frameGraph.AddPass("postProcessPass");
		postPass.AddTextureInput("color");
		postPass.AddTextureInput("brightPass");
		postPass.AddTextureInput("upsample4");
		postPass.AddTextureInput("cloudsTexture");
		//postPass.AddTextureInput("computeImg", computeMat->textures[0]);
		postPass.AddDepthInput("depth");

#ifdef EDITOR
		Engine::AttachmentInfo postOutput = {};
		postOutput.width = game->GetRenderer()->GetWidth();
		postOutput.height = game->GetRenderer()->GetHeight();
		postOutput.params = { Engine::TextureWrap::CLAMP_TO_EDGE, Engine::TextureFilter::LINEAR, Engine::TextureFormat::RGBA, Engine::TextureInternalFormat::RGBA8, Engine::TextureDataType::UNSIGNED_BYTE, false, false };

		postPass.AddTextureOutput("final", postOutput);
#endif

		postPass.OnSetup([this](const Pass *thisPass)
		{
			postProcessFB = thisPass->GetFramebuffer();
			postProcMatInstance = renderer->CreateMaterialInstanceFromBaseMat(game->GetScriptManager(), "Data/Materials/post_process_mat.lua", quadMesh.vao->GetVertexInputDescs());
			postProcMatInstance->textures[0] = hdrFB->GetColorTextureByIndex(0);
			postProcMatInstance->textures[1] = hdrFB->GetDepthTexture();
			postProcMatInstance->textures[2] = upsampleFB[1]->GetColorTexture();
			postProcMatInstance->textures[3] = volumetricClouds.GetCloudsTexture();
			//postProcMatInstance->textures[4] = computeMat->textures[0];
			renderer->UpdateMaterialInstance(postProcMatInstance);
			postProcPassID = postProcMatInstance->baseMaterial->GetShaderPassIndex("postProcessPass");

			// Text
			font.Init(this->renderer, this->game->GetScriptManager(), "Data/Resources/Textures/jorvik.fnt", "Data/Resources/Textures/jorvik.png");
			font.Resize(width, height);

			//const std::vector<VertexInputDesc> &quadDesc = quadMesh.vao->GetVertexInputDescs();

			//debugMatInstance = this->renderer->CreateMaterialInstanceFromBaseMat(this->game->GetScriptManager(), "Data/Resources/Materials/debug_mat.lua", quadDesc);
			//debugMatInstance->textures[0] = csmFB->GetDepthTexture();
			//this->renderer->UpdateMaterialInstance(debugMatInstance);
		});
		postPass.OnResized([this](const Pass *thisPass)
		{
			postProcMatInstance->textures[0] = hdrFB->GetColorTextureByIndex(0);
			postProcMatInstance->textures[1] = hdrFB->GetDepthTexture();
			postProcMatInstance->textures[2] = upsampleFB[1]->GetColorTexture();
		});
		postPass.OnExecute([this]() { PerformPostProcessPass(); });
	}

	void ForwardPlusRenderer::PerformHDRPass()
	{
		renderer->SetCamera(mainCamera);
		renderer->BindImage(LIGHT_GRID_TEXTURE, 0, lightGrid, ImageAccess::READ_ONLY);
		renderer->RebindTexture(csmFB->GetDepthTexture());

		tod.Render(renderer);
		renderer->Submit(renderQueues[4]);

		if (debugSettings.enableVoxelVis)
			vctgi.RenderVoxelVisualization(debugSettings.mipLevel);

		projectedGridWater.Render(renderer);

		// Transparent
		renderer->Submit(renderQueues[5]);
	}

	void ForwardPlusRenderer::PerformPostProcessPass()
	{
		RenderItem ri = {};
		ri.mesh = &quadMesh;
		ri.matInstance = postProcMatInstance;
		ri.shaderPass = postProcPassID;

		renderer->Submit(ri);

		// UI
		if (enableUI)
		{
			renderer->SetCamera(&uiCamera);
			renderer->Submit(renderQueues[6]);

			/*#ifdef EDITOR
			Rect r = {};
			r.position = glm::vec2(width * 0.5f - (width * 0.5f * 0.95f), height / 2);
			r.size = glm::vec2(0.3f, 0.3f);

			font.AddText("Draw calls: " + std::to_string(renderer->GetRendererStats().drawCalls), r);

			r.position = glm::vec2(width * 0.5f - (width * 0.5f * 0.95f), height * 0.5f - (height * 0.5f * 0.06f));
			font.AddText("Dispatch calls: " + std::to_string(renderer->GetRendererStats().dispatchCalls), r);

			r.position = glm::vec2(width * 0.5f - (width * 0.5f * 0.95f), height * 0.5f - (height * 0.5f * 0.12f));
			font.AddText("Triangles count: " + std::to_string(renderer->GetRendererStats().triangles), r);

			r.position = glm::vec2(width * 0.5f - (width * 0.5f * 0.95f), height * 0.5f - (height * 0.5f * 0.18f));
			font.AddText("Instance count: " + std::to_string(renderer->GetRendererStats().instanceCount), r);

			r.position = glm::vec2(width * 0.5f - (width * 0.5f * 0.95f), height * 0.5f - (height * 0.5f * 0.24f));
			font.AddText("Shader changes: " + std::to_string(renderer->GetRendererStats().shaderChanges), r);

			r.position = glm::vec2(width * 0.5f - (width * 0.5f * 0.95f), height * 0.5f - (height * 0.5f * 0.30f));
			font.AddText("Texture changes: " + std::to_string(renderer->GetRendererStats().textureChanges), r);

			r.position = glm::vec2(width * 0.5f - (width * 0.5f * 0.95f), height * 0.5f - (height * 0.5f * 0.36f));
			font.AddText("Culling changes: " + std::to_string(renderer->GetRendererStats().cullingChanges), r);

			#endif*/
			Rect r = {};
			r.position = glm::vec2(width / 2, height / 2);
			r.size = glm::vec2(1.0f);
			//font.AddText(std::to_string(this->renderer->GetFrameTime()), r.position, r.size);
			//font.AddText("hello", r.position, r.size);
			//font.AddText(std::to_string(tod.GetSunriseTime()), glm::vec2(10.0f, 540.0f), glm::vec2(0.25f));
			//font.AddText(std::to_string(tod.GetSunsetTime()), glm::vec2(10.0f, 520.0f), glm::vec2(0.25f));
			//font.AddText(std::to_string(tod.GetCurrentTime()), glm::vec2(10.0f, 500.0f), glm::vec2(0.25f));

			// Text
			font.PrepareText();
			unsigned int curCharCount = font.GetCurrentCharCount();
			if (curCharCount > 0)
			{
				RenderItem ri = {};
				ri.mesh = &font.GetMesh();
				ri.matInstance = font.GetMaterialInstance();
				ri.shaderPass = 0;
				renderer->Submit(ri);
			}
			font.EndTextUpdate();
		}
	}

}