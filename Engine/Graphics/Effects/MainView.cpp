#include "MainView.h"

#include "Graphics\Renderer.h"
#include "Graphics\VertexArray.h"
#include "Graphics\Buffers.h"
#include "Graphics\ResourcesLoader.h"
#include "Graphics\Material.h"
#include "Graphics\MeshDefaults.h"
#include "Graphics\Model.h"

#include "Program\Serializer.h"
#include "Program\Input.h"
#include "Program\StringID.h"
#include "Program\Utils.h"
#include "Program\Random.h"

#include "Game\Game.h"

#include "include\glm\gtc\matrix_transform.hpp"
#include "include\glm\gtc\noise.hpp"

#include <iostream>

namespace Engine
{
	MainView::MainView()
	{
		waterMesh = {};
		lightningMesh = {};

		enableUI = true;
		wireframe = false;

		/*Serializer s;
		s.OpenForReading("Data/post_process.data");
		if (s.IsOpen())
		{
			s.Read(mainDirectionalLight.direction);
			s.Read(mainDirectionalLight.color);
			s.Read(mainDirectionalLight.intensity);
			s.Read(mainDirectionalLight.ambient);
			s.Read(ppd.lightShaftsColor);
			s.Read(ppd.lightShaftsIntensity);
			s.Read(ppd.bloomIntensity);
			s.Read(ppd.bloomThreshold);
			s.Read(ppd.fogParams);
			s.Read(ppd.fogInscatteringColor);
			s.Read(ppd.lightInscatteringColor);
			s.Close();
		}*/
	}

	MainView::~MainView()
	{
		if (waterMesh.vao)
			delete waterMesh.vao;
		//if(skyboxMesh.vao)
		//	delete skyboxMesh.vao;
		if (lightningMesh.vao)
			delete lightningMesh.vao;
	}

	void MainView::Render()
	{
		if (Input::WasKeyReleased(KEY_R))
		{
			performStrike = true;
		}

		/*if (performStrike)
		{
			float deltaTime = game->GetDeltaTime();

			lightningTimer += deltaTime;


			if (lightningAlpha < 0.4f)
				lightningAlpha += deltaTime * 8.0f;
			else
				lightningAlpha = std::sin(Random::Float(-1.0f, 1.0f));

			//lightIntensity = lightningAlpha * 0.5f + 0.5f;

			if (lightningTimer >= 0.35f)
			{
				performStrike = false;
				lightningTimer = 0.0f;
				lightningAlpha = -1.0f;
				//lightIntensity = 0.0f;
			}
		}*/
	/*	else
		{
			if (lightningTimer > 0.0f)
			{
				if (lightningAlpha > -1.0f)
					lightningAlpha -= game->GetDeltaTime() * 8.0f;

				lightningTimer -= game->GetDeltaTime();
			}
		}*/
		
		

		/*if (debugSettings.enableWater)
		{
			// Render the reflection
			viewport.width = width / 2;
			viewport.height = height / 2;
			renderer->SetRenderTargetAndClear(reflectionFBO);
			renderer->SetViewport(viewport);

			glm::vec3 camPos = mainCamera->GetPosition();
			float distance = 2.0f * (camPos.y - 12.0f);
			camPos.y -= distance;
			mainCamera->SetPosition(camPos);
			float pitch = mainCamera->GetPitch();
			mainCamera->SetPitch(-pitch);		// Invert the pitch

			renderer->SetCamera(mainCamera, glm::vec4(0.0f, 12.0f + 0.5f, 0.0f, 1.0f));

			// Render the skybox 
			/*ri.mesh = &skyboxMesh;
			ri.matInstance = skyboxMatIntance;
			ri.shaderPass = skyboxShaderPass;
			renderer->Render(ri);

			//renderer->Render(renderQueues[0]);

			//timeOfDayManager.Render(renderer);

			//if (renderQueues[1].size() > 0)
			//	renderer->Render(renderQueues[1][renderQueues[1].size() - 1]);

			/*camPos.y += distance;
			mainCamera->SetPosition(camPos);
			mainCamera->SetPitch(pitch);

			renderer->EndRenderTarget(reflectionFBO, false);
		//}*/

		// Voxel Cone Tracing Global Illumination
		
		//vctgi.Voxelize(renderQueues[4]);

/*#ifdef EDITOR
		// Debug draw
		if (debugSettings.enableDebugDraw)
		{
			// Render the debug draw objects after we perform bloom so we don't have the boxes and lines glowing all over the place
			renderer->SetViewport(viewport);		// Reset the viewport (we changed in the bloom pass)
			renderer->SetRenderTarget(hdrFBO);		// Set the render target but don't clear it's contents
			renderer->Render(renderQueues[3]);
		}
		//renderer->EndRenderTarget(hdrFBO, false);
#endif*/
	}

	void MainView::PerformHDRPass()
	{
		renderer->SetCamera(mainCamera);
		//renderer->SetCSMTexture(csmFB->GetDepthTexture());

		//timeOfDayManager.Render(renderer);
		//vctgi.GetVoxelTexture()->Bind(7);
		renderer->Submit(renderQueues[4]);
		
		// Water
		/*if (debugSettings.enableWater)
		{
		ri.mesh = &waterMesh;
		ri.matInstance = waterMatInstance;
		ri.shaderPass = waterShaderPass;
		glm::mat4 m = glm::translate(glm::mat4(1.0f), glm::vec3(256.0f, 38.0f, 256.0f));
		m = glm::scale(m, glm::vec3(512.0f, 1.0f, 512.0f));
		ri.transform = m;

		moveFactor += 0.035f * game->GetDeltaTime();
		moveFactor = fmod(moveFactor, 1.0f);
		glm::vec2 waterParams = glm::vec2(moveFactor, waveStrength);

		renderer->Render(ri, &waterParams.x, sizeof(waterParams));
		}

		//if (debugSettings.enableVoxelVis)
		//	vctgi.RenderVoxelVisualization((unsigned int)debugSettings.mipLevel);
		*/
		// Render the skydome before the transparent objects otherwise there are artifacts
		
		//vctgi.RenderVoxelVisualization(0);

		//projectedGridWater.Render(renderer);

		// Transparent
		renderer->Submit(renderQueues[5]);

		/*RenderItem ri = {};
		ri.mesh = &lightningMesh;
		ri.matInstance = lightningMatInstance;
		ri.shaderPass = 0;
		ri.materialData = &lightningAlpha;
		ri.materialDataSize = sizeof(lightningAlpha);
		renderer->Render(ri);*/
	}

	void MainView::PerformPostProcessPass()
	{
		/*ri.mesh = &quadMesh;
		ri.matInstance = quadMat;
		ri.shaderPass = postProcPassID;
		renderer->Submit(ri);*/




		// AFTER UI

		// Debug quad
		/*if (debugSettings.enable)
		{
			debugMatData = {};

			if (debugSettings.type == DebugType::CSM_SHADOW_MAP)
			{
				debugMatData.isShadowMap = 1;
				if (debugMatInstance->textures[0] != csmFB->GetDepthTexture())
				{
					debugMatInstance->textures[0] = csmFB->GetDepthTexture();
					renderer->UpdateMaterialInstance(debugMatInstance);
				}
			}
			else if (debugSettings.type == DebugType::BRIGHT_PASS)
			{
				if (debugMatInstance->textures[0] != brightPassFB->GetColorTexture())
				{
					debugMatInstance->textures[0] = brightPassFB->GetColorTexture();
					renderer->UpdateMaterialInstance(debugMatInstance);
				}
			}
			else if (debugSettings.type == DebugType::REFLECTION)
			{
				if (debugMatInstance->textures[0] != reflectionFB->GetColorTexture())
				{
					debugMatInstance->textures[0] = reflectionFB->GetColorTexture();
					renderer->UpdateMaterialInstance(debugMatInstance);
				}
			}
			else if (debugSettings.type == DebugType::REFRACTION)
			{
				if (debugMatInstance->textures[0] != refractionFB->GetColorTexture())
				{
					debugMatInstance->textures[0] = refractionFB->GetColorTexture();
					renderer->UpdateMaterialInstance(debugMatInstance);
				}
			}
			
			RenderItem ri = {};
			//ri.mesh = &quadMesh;
			ri.matInstance = debugMatInstance;
			ri.shaderPass = 0;
			ri.materialData = &debugMatData;
			ri.materialDataSize = sizeof(DebugMaterialData);

			renderer->Submit(ri);
		}*/
		/*
		struct data
		{
		int isShadowMap;
		int isVoxelTexture;
		float voxelZSlice;
		int mipLevel;
		int voxelRes;
		};

		data d = {};

		if (debugSettings.type == DebugType::CSM_SHADOW_MAP)
		{
		d.isShadowMap = 1;
		debugMatInstance->textures[0] = csmInfo.rt->GetDepthTexture();
		}
		else if (debugSettings.type == DebugType::BRIGHT_PASS)
		debugMatInstance->textures[0] = brightPassFBO->GetColorTexture();
		else if (debugSettings.type == DebugType::REFLECTION)
		debugMatInstance->textures[0] = reflectionFBO->GetColorTexture();
		else if (debugSettings.type == DebugType::VOXEL_TEXTURE)
		{
		d.mipLevel = debugSettings.mipLevel;
		d.voxelZSlice = (float)debugSettings.zSlice;
		d.isVoxelTexture = 1;
		d.voxelRes = 128 >> d.mipLevel;
		debugMatInstance->textures[1] = vctgi.GetVoxelTexture();
		}

		renderer->Render(ri, &d, sizeof(d));
		}*/
	}

	void MainView::GenerateLightning()
	{
		glm::vec3 v = glm::vec3(0.0f);
		float heightIncrease = 0.5f;
		float xOffset = Random::Float(0.0f, 3.14f);
		float zOffset = Random::Float(0.0f, 3.14f);
		float leaderIntensity = 1.5f;
		float lightningHeight = (heightIncrease + divisions);

		lightningVertices.clear();
		lightningVertices.push_back({ glm::vec3(0.0f), leaderIntensity });

		for (unsigned int i = 0; i < divisions; i++)
		{
			v.x = Random::Float(-0.5f, 0.5f) + std::sin(xOffset) * 2.0f;
			v.y = Random::Float(0.5f, 1.5f) + heightIncrease;
			v.z = Random::Float(-0.5f, 0.5f) + std::cos(zOffset) * 2.0f;
			lightningVertices.push_back({ v, leaderIntensity });

			if (Random::Float() > 0.32f && i > 3)
			{
				float heightGradient = (float)i / lightningHeight;
				float lengthMultiplier = glm::mix(0.15f, 2.5f, heightGradient);		// For the branches to be larger on the top and smaller on the bottom

				branchLevel = 0;
				AddBranch(v, lengthMultiplier, leaderIntensity - 0.2f);
			}

			if (i < divisions - 1)
				lightningVertices.push_back({ v, leaderIntensity });			// Push v again for the next line of the main leader to render correctly

			heightIncrease += 1.0f;
			xOffset += 0.5f;
			zOffset += 0.3f;
		}

		VertexAttribute pos = {};
		pos.count = 3;
		pos.vertexAttribFormat = VertexAttributeFormat::FLOAT;
		VertexAttribute intensity = {};
		intensity.count = 1;
		intensity.offset = 3 * sizeof(float);
		intensity.vertexAttribFormat = VertexAttributeFormat::FLOAT;

		VertexInputDesc desc = {};
		desc.attribs = { pos, intensity };
		desc.stride = 4 * sizeof(float);

		Buffer *vb = renderer->CreateVertexBuffer(lightningVertices.data(), lightningVertices.size() * sizeof(LightningVertex), BufferUsage::STATIC);
		lightningMesh.vao = renderer->CreateVertexArray(desc, vb, nullptr);
		lightningMesh.vertexCount = (unsigned int)lightningVertices.size();

		lightningMatInstance = renderer->CreateMaterialInstanceFromBaseMat(game->GetScriptManager(), "Data/Resources/Materials/lightning_mat.lua", { desc });
		std::cout << "erejh\n";
	}

	void MainView::AddBranch(const glm::vec3 &startPoint, float lengthMultiplier, float startingBranchIntensity)
	{
		branchLevel++;

		glm::vec3 branch = glm::vec3(startPoint.x + Random::Float(-0.5f, 0.5f), startPoint.y - Random::Float(0.5f, 1.0f), startPoint.z /*+ Random::Float(-0.5f, 0.5f)*/);

		lightningVertices.push_back({ startPoint, startingBranchIntensity });
		lightningVertices.push_back({ branch, startingBranchIntensity });

		glm::vec3 dir = glm::normalize(branch - startPoint) * lengthMultiplier;

		float inv = 1.0f / 32.5f;

		float heightGradient = branch.y / 32.5f;
		float startLengthMult = 0.8f / branchLevel;
		float startHeightDecrease = 0.9f;
		float heightDecrease = startHeightDecrease * heightGradient;
		float branchIntensity = startingBranchIntensity - 0.15f;

		unsigned int nextID = lightningVertices.size() - 1;

		for (int i = 0; i < 16 / branchLevel; i++)
		{
			const LightningVertex &last = lightningVertices[nextID];
			glm::vec3 branchSegment = glm::vec3(dir.x * startLengthMult + last.pos.x, last.pos.y - heightDecrease + Random::Float(-1.0f * heightGradient, 1.0f * heightGradient), /* dir.z * startLengthMult +*/ last.pos.z);
			dir = glm::normalize(branchSegment - last.pos) * lengthMultiplier;
			//dir.y -= 0.05f;

			lightningVertices.push_back(lightningVertices[nextID]);		// Push back the last so we continue the branch where the last segment ended
			lightningVertices.push_back({ branchSegment, branchIntensity });

			float heightGradient = branchSegment.y * inv;
			heightDecrease = startHeightDecrease * heightGradient;

			//heightDecrease += 0.04f;
			startLengthMult -= 0.04f;
			branchIntensity -= 0.08f;

			nextID = lightningVertices.size() - 1;

			if (Random::Float() > 0.3f && i > 3 && i < 5)
			{
				if (branchLevel < 2)
					AddBranch(branchSegment, lengthMultiplier, startingBranchIntensity);
			}
		}
	}
}
