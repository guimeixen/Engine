#include "ForwardRenderer.h"

#include "Game/Game.h"
#include "Graphics/Renderer.h"
#include "Graphics/Buffers.h"
#include "Graphics/Material.h"
#include "Graphics/MeshDefaults.h"
#include "Graphics/VertexArray.h"

namespace Engine
{
	ForwardRenderer::ForwardRenderer()
	{
		renderingPathType = RenderingPathType::FORWARD;
	}

	void ForwardRenderer::Init(Game *game)
	{
		RenderingPath::Init(game);

		pointLightsUBO = renderer->CreateUniformBuffer(nullptr, sizeof(PointLights));

		SetupHDRPass();
		SetupPostProcessPass();

		frameGraph.SetBackbufferSource("postProcessPass");		// Post process pass writes to backbuffer
		frameGraph.Bake(renderer);
		frameGraph.ExportGraphVizFile();
	
		renderer->AddResourceToSlot(4, frameGraph.GetPass("csm").GetFramebuffer()->GetDepthTexture(), false, PipelineStage::FRAGMENT);
		renderer->AddResourceToSlot(5, pointLightsUBO, PipelineStage::VERTEX | PipelineStage::FRAGMENT);

		renderer->SetupResources();	
	}

	void ForwardRenderer::Dispose()
	{
		RenderingPath::Dispose();
	}

	void ForwardRenderer::Resize(unsigned int width, unsigned int height)
	{
		RenderingPath::Resize(width, height);

		frameGraph.Bake(renderer);

		//volumetricClouds.Resize(width, height, frameGraph);

		renderer->UpdateMaterialInstance(postProcMatInstance);
		renderer->UpdateMaterialInstance(brightPassMatInstance);
		renderer->UpdateMaterialInstance(downsample4MatInstance);
		renderer->UpdateMaterialInstance(downsample8MatInstance);
		renderer->UpdateMaterialInstance(downsample16MatInstance);
		renderer->UpdateMaterialInstance(upsample8MatInstance);
		renderer->UpdateMaterialInstance(upsample4MatInstance);
		renderer->UpdateMaterialInstance(fxaaMat);
	}

	void ForwardRenderer::Render()
	{
		RenderingPath::Render();

		const PointLightUBO plUBO = game->GetLightManager().GetPointLightsUBO();
		pointLightsUBO->Update(&plUBO, sizeof(plUBO), 0);


		unsigned int queueIDs[] = { csmQueueID, csmQueueID, csmQueueID, voxelizationQueueID, opaqueQueueID, transparentQueueID, uiQueueID };

		const glm::vec3 &camPos = mainCamera->GetPosition();
		float voxelGridSize = vctgi.GetVoxelGridSize();
		float voxelGridHalfSize = voxelGridSize * 0.5f;

		Frustum ortho;
		ortho.UpdateProjection(camPos.x - voxelGridHalfSize, camPos.x + voxelGridHalfSize, camPos.y - voxelGridHalfSize, camPos.y + voxelGridHalfSize, camPos.z - voxelGridHalfSize, camPos.z + voxelGridHalfSize);
		ortho.Update(camPos, camPos + mainCamera->GetFront(), mainCamera->GetUp());

		Frustum frustums[7];
		frustums[0] = csmInfo.cameras[0].GetFrustum();
		frustums[1] = csmInfo.cameras[1].GetFrustum();
		frustums[2] = csmInfo.cameras[2].GetFrustum();
		frustums[3] = ortho;			// TODO: Create an orthographic frustum for this pass
		frustums[4] = mainCamera->GetFrustum();
		frustums[5] = mainCamera->GetFrustum();
		frustums[6] = uiCamera.GetFrustum();
		
		std::vector<VisibilityIndices> visibility = renderer->Cull(7, queueIDs, frustums);

		renderer->CreateRenderQueues(7, queueIDs, visibility, renderQueues);
		//renderer->CreateRenderQueues(7, queueIDs, frustums, renderQueues);

		vctgi.EndFrame();
		frameGraph.Execute(renderer);

		frameData.previousFrameView = mainCamera->GetViewMatrix();

		volumetricClouds.EndFrame();
	}

	void ForwardRenderer::SetupHDRPass()
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

		hdrPass.AddImageInput("voxelTextureMipmapped", vctgi.GetVoxelTexture(), true);
		hdrPass.AddBufferInput("voxelsIndirectBuffer", vctgi.GetIndirectBuffer());
		hdrPass.AddBufferInput("voxelPositionsBuffer", vctgi.GetVoxelPositionsBuffer());
		hdrPass.AddDepthInput("shadowMap");
		hdrPass.AddTextureInput("reflectionTex");
		hdrPass.AddTextureInput("refractionTex");
		hdrPass.AddDepthInput("refractionDepth");
		hdrPass.AddTextureOutput("color", colorAttachment);
		hdrPass.AddDepthOutput("depth", depthAttachment);

		hdrPass.SetOnSetup([this](const Pass *thisPass)
		{
			hdrFB = thisPass->GetFramebuffer();

			tod.Init(game);

			//GenerateLightning();
			vctgi.CreateMat(game->GetScriptManager());
		});


		hdrPass.SetOnBarriers([this]()
		{
			Barrier b = {};

			/*BarrierImage bi = {};
			bi.image = vctgi.GetVoxelTexture();
			bi.readToWrite = false;
			bi.transitionToShaderRead = true;*/

			BarrierBuffer bb1 = {};
			bb1.buffer = vctgi.GetIndirectBuffer();
			bb1.readToWrite = false;

			BarrierBuffer bb2 = {};
			bb2.buffer = vctgi.GetVoxelPositionsBuffer();
			bb2.readToWrite = false;

			//b.images.push_back(bi);
			b.buffers.push_back(bb1);
			b.buffers.push_back(bb2);
			b.srcStage = PipelineStage::COMPUTE;
			b.dstStage = PipelineStage::INDIRECT | PipelineStage::VERTEX;

			renderer->PerformBarrier(b);
		});

		hdrPass.SetOnExecute([this]() {PerformHDRPass(); });
	}

	void ForwardRenderer::SetupPostProcessPass()
	{
		Pass &postPass = frameGraph.AddPass("postProcessPass");
		postPass.AddTextureInput("color");
		postPass.AddTextureInput("brightPass");
		postPass.AddTextureInput("upsample4");
		postPass.AddTextureInput("cloudsTexture");
		//postPass.AddTextureInput("computeImg", computeMat->textures[0]);
		postPass.AddDepthInput("depth");

		postPass.SetOnSetup([this](const Pass *thisPass)
		{
			postProcessFB = thisPass->GetFramebuffer();
			postProcMatInstance = renderer->CreateMaterialInstanceFromBaseMat(game->GetScriptManager(), "Data/Resources/Materials/post_process_mat.lua", quadMesh.vao->GetVertexInputDescs());
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
		postPass.SetOnResized([this](const Pass *thisPass)
		{
			postProcMatInstance->textures[0] = hdrFB->GetColorTextureByIndex(0);
			postProcMatInstance->textures[1] = hdrFB->GetDepthTexture();
			postProcMatInstance->textures[2] = upsampleFB[1]->GetColorTexture();
		});
		postPass.SetOnExecute([this]() { PerformPostProcessPass(); });

#ifdef EDITOR
		Engine::AttachmentInfo finalAttach = {};
		finalAttach.width = game->GetRenderer()->GetWidth();
		finalAttach.height = game->GetRenderer()->GetHeight();
		finalAttach.params = { Engine::TextureWrap::CLAMP_TO_EDGE, Engine::TextureFilter::LINEAR, Engine::TextureFormat::RGBA, Engine::TextureInternalFormat::RGBA8, Engine::TextureDataType::UNSIGNED_BYTE, false, false };

		postPass.AddTextureOutput("final", finalAttach);
#endif
	}

	void ForwardRenderer::PerformHDRPass()
	{
		renderer->SetCamera(mainCamera);
		renderer->RebindTexture(csmFB->GetDepthTexture());

		tod.Render(renderer);
		renderer->Submit(renderQueues[4]);

		if (debugSettings.enableVoxelVis)
			vctgi.RenderVoxelVisualization(debugSettings.mipLevel);

		projectedGridWater.Render(renderer);

		// Transparent
		renderer->Submit(renderQueues[5]);
	}

	void ForwardRenderer::PerformPostProcessPass()
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
			font.AddText(std::to_string(tod.GetSunriseTime()), glm::vec2(10.0f, 540.0f), glm::vec2(0.25f));
			font.AddText(std::to_string(tod.GetSunsetTime()), glm::vec2(10.0f, 520.0f), glm::vec2(0.25f));
			font.AddText(std::to_string(tod.GetCurrentTime()), glm::vec2(10.0f, 500.0f), glm::vec2(0.25f));

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