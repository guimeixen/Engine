#include "PSVitaRenderer.h"

#include "Game/Game.h"
#include "Program/Log.h"
#include "Program/StringID.h"
#include "Program/Input.h"
#include "Graphics/MeshDefaults.h"
#include "Graphics/VertexArray.h"

#include "Data/Shaders/GXM/include/common.cgh"

#include "include/glm/gtc/matrix_transform.hpp"

namespace Engine
{
	PSVitaRenderer::PSVitaRenderer()
	{
	}

	void PSVitaRenderer::Init(Game *game)
	{
		Log::Print(LogLevel::LEVEL_INFO, "Init PSVita renderer\n");

		this->game = game;
		renderer = game->GetRenderer();
		width = renderer->GetWidth();
		height = renderer->GetHeight();

		csmQueueID = SID("csm");
		opaqueQueueID = SID("opaque");

		quadMesh = MeshDefaults::CreateQuad(renderer);

		// UI
		uiCamera.SetFrontAndUp(glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, 1.0f, 0.0f));
		uiCamera.SetPosition(glm::vec3(0.0f));
		uiCamera.SetProjectionMatrix(0.0f, (float)renderer->GetWidth(), 0.0f, (float)renderer->GetHeight(), 0.0f, 10.0f);

		mainLightUBO = renderer->CreateUniformBuffer(nullptr, sizeof(DirLightUBOSimple));

		SetupShadowMappingPass();
		SetupHDRPass();
		SetupPostProcessPass();

		frameGraph.SetBackbufferSource("postProcessPass");
		frameGraph.Bake(renderer);
		frameGraph.Setup();

		renderer->AddBufferResourceToSlot(MAIN_LIGHT_UBO_SLOT, mainLightUBO, PipelineStage::VERTEX | PipelineStage::FRAGMENT);
		renderer->AddTextureResourceToSlot(0, frameGraph.GetPass("csm").GetFramebuffer()->GetDepthTexture(), false, PipelineStage::FRAGMENT);
	}

	void PSVitaRenderer::Dispose()
	{
		font.Dispose();
		frameGraph.Dispose();
	}

	void PSVitaRenderer::Render()
	{
		//tod.Update(game->GetDeltaTime());
		const TimeInfo &t = tod.GetCurrentTimeInfo();

		glm::vec3 mainLightDir = glm::vec3(0.0f);
		if (lockMainLightToTOD)
		{
			mainLightDir = glm::normalize(t.dirLightDirection);
		}
		else
		{
			mainLightDir = glm::normalize(mainDirectionalLight.direction);
		}

		// Update the lighting ubo
		DirLightUBOSimple ubo = {};
		ubo.lightSpaceMatrix = glm::ortho(-10.0f, 10.0f, -10.0f, 10.0f, 0.1f, 20.0f) * glm::lookAt(glm::vec3(5.0f, 5.0f, 5.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));

		// This matrix does the equivalent of projCoords.xy = projCoords * 0.5 + 0.5 and then flipping the y    projCoords.y = 1.0 - projCoords.y
		// Like this we don't have to do that in the shader for every fragment
		glm::mat4 m = glm::mat4(1.0f);
		m[0] = glm::vec4(0.5f, 0.0f, 0.0f, 0.0f);
		m[1] = glm::vec4(0.0f, -0.5f, 0.0f, 0.0f);
		m[2] = glm::vec4(0.0f, 0.0f, 0.5f, 0.0f);
		m[3] = glm::vec4(0.5f, 0.5f, 0.5f, 1.0f);

		ubo.lightSpaceMatrix = m * ubo.lightSpaceMatrix;
		ubo.dirAndIntensity = glm::vec4(mainLightDir, mainDirectionalLight.intensity);
		ubo.dirLightColor = glm::vec4(mainDirectionalLight.color, mainDirectionalLight.ambient);
		/*ubo.skyColor = glm::vec4(1.0f);
		ubo.lightSpaceMatrix[0] = glm::mat4(1.0f);
		ubo.lightSpaceMatrix[1] = glm::mat4(1.0f);
		ubo.lightSpaceMatrix[2] = glm::mat4(1.0f);
		ubo.lightSpaceMatrix[3] = glm::mat4(1.0f);
		ubo.cascadeEnd = glm::vec4(csmInfo.cascadeSplitEnd[0], csmInfo.cascadeSplitEnd[1], csmInfo.cascadeSplitEnd[2], 0.0f);*/

		mainLightUBO->Update(&ubo, sizeof(DirLightUBOSimple), 0);

		unsigned int queueIDs[] = { csmQueueID, opaqueQueueID };

		std::vector<VisibilityIndices> visibility = renderer->Cull(2, queueIDs, &mainCamera->GetFrustum());
		renderer->CreateRenderQueues(2, queueIDs, visibility, renderQueues);
		frameGraph.Execute(renderer);
	}

	void PSVitaRenderer::SetupShadowMappingPass()
	{
		Pass &csmPass = frameGraph.AddPass("csm");
		AttachmentInfo shadowMap = {};
		shadowMap.width = 256;
		shadowMap.height = 256;
		//shadowMap.params = { TextureWrap::CLAMP_TO_BORDER, TextureFilter::LINEAR, TextureFormat::DEPTH_COMPONENT, TextureInternalFormat::DEPTH_COMPONENT32, TextureDataType::FLOAT };
		shadowMap.params = { TextureWrap::CLAMP_TO_BORDER, TextureFilter::LINEAR, TextureFormat::DEPTH_COMPONENT, TextureInternalFormat::DEPTH_COMPONENT32, TextureDataType::FLOAT };
		csmPass.AddDepthOutput("shadowMap", shadowMap);

		csmPass.OnSetup([this](const Pass *thisPass)
		{
			//debugMatInstance->textures[0] = thisPass->GetFramebuffer()->GetDepthTexture();
			csmFB = thisPass->GetFramebuffer();
		});

		csmPass.OnExecute([this]()
		{
			Camera cam;
			cam.SetProjectionMatrix(-10.0f, 10.0f, -10.0f, 10.0f, 0.1f, 20.0f);
			cam.SetViewMatrix(glm::vec3(5.0f, 5.0f, 5.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));

			renderer->SetCamera(&cam);
			renderer->Submit(renderQueues[0]);
		});
	}

	void PSVitaRenderer::SetupHDRPass()
	{
		AttachmentInfo colorAttachment = {};
		colorAttachment.width = renderer->GetWidth();
		colorAttachment.height = renderer->GetHeight();
		colorAttachment.params = { TextureWrap::CLAMP_TO_EDGE, TextureFilter::LINEAR, TextureFormat::RGBA, TextureInternalFormat::RGBA8, TextureDataType::UNSIGNED_BYTE };

		AttachmentInfo depthAttachment = {};
		depthAttachment.width = renderer->GetWidth();
		depthAttachment.height = renderer->GetHeight();
		depthAttachment.params = { TextureWrap::CLAMP_TO_EDGE, TextureFilter::LINEAR, TextureFormat::DEPTH_COMPONENT, TextureInternalFormat::DEPTH_COMPONENT24, TextureDataType::FLOAT };


		Pass &hdrPass = frameGraph.AddPass("default");
		hdrPass.AddTextureOutput("hdrTexture", colorAttachment);
		hdrPass.AddDepthOutput("hdrDepth", depthAttachment);
		hdrPass.AddDepthInput("shadowMap");

		hdrPass.OnSetup([this](const Pass *thisPass)
		{
			hdrFB = thisPass->GetFramebuffer();
		});

		hdrPass.OnExecute([this]()
		{
			renderer->SetCamera(mainCamera);
			renderer->Submit(renderQueues[1]);
		});
	}

	void PSVitaRenderer::SetupPostProcessPass()
	{
		Pass &postProcessPass = frameGraph.AddPass("postProcessPass");
		postProcessPass.AddTextureInput("hdrTexture");

		postProcessPass.OnSetup([this](const Pass *thisPass)
		{
			postProcessFB = thisPass->GetFramebuffer();
			postProcMatInstance = renderer->CreateMaterialInstanceFromBaseMat(this->game->GetScriptManager(), "Data/Materials/post_process_mat.lua", quadMesh.vao->GetVertexInputDescs());
			postProcMatInstance->textures[0] = hdrFB->GetColorTextureByIndex(0);

			font.Init(renderer, this->game->GetScriptManager(), "Data/Resources/Textures/jorvik.fnt", "Data/Resources/Textures/jorvik.png");
		});

		postProcessPass.OnExecute([this]()
		{
			RenderItem ri = {};
			ri.mesh = &quadMesh;
			ri.matInstance = postProcMatInstance;
			ri.shaderPass = 0;

			renderer->Submit(ri);

			renderer->SetCamera(&uiCamera);

			//font.AddText(std::to_string(this->game->GetDeltaTime()), glm::vec2(50.0f, 50.0f), glm::vec2(0.5f));
			//glm::vec3 f = this->mainCamera->GetFront();
			//font.AddText(std::to_string(f.x) + ' ' + std::to_string(f.y) + ' ' + std::to_string(f.z), glm::vec2(40.0f, 80.0f), glm::vec2(0.4f));
			font.AddText(std::to_string(Input::GetLeftAnalogueStickX()) + ' ' + std::to_string(Input::GetLeftAnalogueStickY()), glm::vec2(40.0f, 100.0f), glm::vec2(0.4f));
			//font.AddText(std::to_string(mainDirectionalLight.direction.x) + ' ' + std::to_string(mainDirectionalLight.direction.y) + ' ' + std::to_string(mainDirectionalLight.direction.z), glm::vec2(40.0f, 100.0f), glm::vec2(0.4f));
			//font.AddText(std::to_string(mainDirectionalLight.intensity), glm::vec2(40.0f, 120.0f), glm::vec2(0.4f));
			//font.AddText(std::to_string(mainDirectionalLight.color.x) + ' ' + std::to_string(mainDirectionalLight.color.y) + ' ' + std::to_string(mainDirectionalLight.color.z), glm::vec2(40.0f, 140.0f), glm::vec2(0.4f));

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
		});
	}
}
