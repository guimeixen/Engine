#include "PSVitaRenderer.h"

#include "Game/Game.h"
#include "Program/Log.h"
#include "Program/Input.h"

#include "Data/Shaders/GXM/include/common.cgh"

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

		// UI
		uiCamera.SetFrontAndUp(glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, 1.0f, 0.0f));
		uiCamera.SetPosition(glm::vec3(0.0f));
		uiCamera.SetProjectionMatrix(0.0f, (float)renderer->GetWidth(), 0.0f, (float)renderer->GetHeight(), 0.0f, 10.0f);

		mainLightUBO = renderer->CreateUniformBuffer(nullptr, sizeof(DirLightUBOSimple));

		renderer->AddResourceToSlot(MAIN_LIGHT_UBO_SLOT, mainLightUBO, PipelineStage::VERTEX | PipelineStage::FRAGMENT);

		Pass &p = frameGraph.AddPass("default");

		p.SetOnSetup([this](const Pass *thisPass)
		{
			font.Init(renderer, this->game->GetScriptManager(), "Data/Resources/Textures/jorvik.fnt", "Data/Resources/Textures/jorvik.png");
		});

		p.SetOnExecute([this]()
		{
			renderer->SetCamera(mainCamera);
			renderer->Submit(renderQueues[0]);

			renderer->SetCamera(&uiCamera);

			//font.AddText(std::to_string(this->game->GetDeltaTime()), glm::vec2(50.0f, 50.0f), glm::vec2(0.5f));
			glm::vec3 f = this->mainCamera->GetFront();
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

		frameGraph.SetBackbufferSource("default");
		frameGraph.Bake(renderer);
		frameGraph.Setup();
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
		ubo.dirAndIntensity = glm::vec4(mainLightDir, mainDirectionalLight.intensity);
		ubo.dirLightColor = glm::vec4(mainDirectionalLight.color, mainDirectionalLight.ambient);
		/*ubo.skyColor = glm::vec4(1.0f);
		ubo.lightSpaceMatrix[0] = glm::mat4(1.0f);
		ubo.lightSpaceMatrix[1] = glm::mat4(1.0f);
		ubo.lightSpaceMatrix[2] = glm::mat4(1.0f);
		ubo.lightSpaceMatrix[3] = glm::mat4(1.0f);
		ubo.cascadeEnd = glm::vec4(csmInfo.cascadeSplitEnd[0], csmInfo.cascadeSplitEnd[1], csmInfo.cascadeSplitEnd[2], 0.0f);*/

		mainLightUBO->Update(&ubo, sizeof(DirLightUBOSimple), 0);

		std::vector<VisibilityIndices> visibility = renderer->Cull(1, &opaqueQueueID, &mainCamera->GetFrustum());
		renderer->CreateRenderQueues(1, &opaqueQueueID, visibility, renderQueues);
		frameGraph.Execute(renderer);
	}
}
