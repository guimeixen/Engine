#include "RenderingPath.h"

#include "Game/Game.h"
#include "Program/StringID.h"
#include "Graphics/MeshDefaults.h"
#include "Graphics/VertexArray.h"
#include "Program/Serializer.h"
#include "Program/Log.h"

#include <iostream>

namespace Engine
{
	RenderingPath::RenderingPath()
	{
		quadMesh = {};
		enableUI = true;

		debugSettings = {};
		debugSettings.enable = false;
		debugSettings.enableVoxelVis = false;
		debugSettings.enableDebugDraw = true;

		mainDirectionalLight = {};
		mainDirectionalLight.intensity = 1.4f;
		mainDirectionalLight.direction = glm::vec3(1.0f, 0.5f, -0.3f);
		mainDirectionalLight.color = glm::vec3(1.0f);
		mainDirectionalLight.ambient = 0.17f;

		frameData.giIntensity = 1.0f;
		frameData.aoIntensity = 0.0f;
		frameData.skyColorMultiplier = 0.55f;
		frameData.enableGI = true;
		frameData.lightShaftsIntensity = 1.0f;
		frameData.lightShaftsColor = glm::vec4(1.0f);
		frameData.bloomIntensity = 1.0f;
		//frameUBO.sampleScale = 1.0f;
		frameData.bloomThreshold = 1.5f;
		//frameUBO.bloomRadius = 1.0f;
		frameData.fogParams.x = 20.5f;			// fog height
		frameData.fogParams.y = 0.05f;			// fog density
		frameData.fogInscatteringColor = glm::vec4(1.0f);
		frameData.lightInscatteringColor = glm::vec4(1.0f);
		frameData.lightShaftsParams.x = 1.0f;	// density
		frameData.lightShaftsParams.y = 1.0f;		// decay
		frameData.lightShaftsParams.z = 5.0f;		// weight
		frameData.lightShaftsParams.w = 0.0034f;		// exposure

		frameData.vignetteParams.x = 0.0f;		// intensity
		frameData.vignetteParams.y = 0.25f;		// falloff
	}

	void RenderingPath::Init(Game *game)
	{
		this->game = game;
		renderer = game->GetRenderer();
		width = renderer->GetWidth();
		height = renderer->GetHeight();

		// UI
		uiCamera.SetFrontAndUp(glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, 1.0f, 0.0f));
		uiCamera.SetPosition(glm::vec3(0.0f));
		uiCamera.SetProjectionMatrix(0.0f, (float)renderer->GetWidth(), 0.0f, (float)renderer->GetHeight(), 0.0f, 10.0f);

		csmQueueID = SID("csm");
		voxelizationQueueID = SID("voxelization");
		opaqueQueueID = SID("opaque");
		transparentQueueID = SID("transparent");
		uiQueueID = SID("ui");
		debugDrawQueueId = SID("debugDrawPass");


		quadMesh = MeshDefaults::CreateQuad(renderer);

		volumetricClouds.Init(renderer, game->GetScriptManager(), frameGraph, quadMesh);
		vctgi.Init(renderer, frameGraph, game->GetScriptManager());

		SetupCSMPass();
		SetupVoxelizationPass();
		SetupBloomPasses();
		//SetupFXAAPass();

		if (renderingPathType == RenderingPathType::FORWARD || renderingPathType == RenderingPathType::FORWARD_PLUS)
		{
			SetupReflectionPass();
			SetupRefractionPass();
		}

		frameUBO = renderer->CreateUniformBuffer(nullptr, sizeof(FrameUBO));
		mainLightUBO = renderer->CreateUniformBuffer(nullptr, sizeof(DirLightUBO));

		// Slot 0 is reserved for the camera ubo and slot 1 for the instance/transform data ssbo
		renderer->AddResourceToSlot(2, frameUBO, PipelineStage::VERTEX | PipelineStage::GEOMETRY | PipelineStage::FRAGMENT | PipelineStage::COMPUTE);
		renderer->AddResourceToSlot(3, mainLightUBO, PipelineStage::VERTEX | PipelineStage::FRAGMENT);
		
		renderer->AddResourceToSlot(6, vctgi.GetVoxelTexture(), true, PipelineStage::VERTEX | PipelineStage::FRAGMENT | PipelineStage::COMPUTE);	// use the voxel texture as a storage image here
		renderer->AddResourceToSlot(7, vctgi.GetIndirectBuffer(), PipelineStage::COMPUTE);
		renderer->AddResourceToSlot(8, vctgi.GetVoxelPositionsBuffer(), PipelineStage::VERTEX | PipelineStage::COMPUTE);
		renderer->AddResourceToSlot(9, vctgi.GetVoxelTexture(), false, PipelineStage::VERTEX | PipelineStage::FRAGMENT | PipelineStage::COMPUTE);	// use the voxel texture as a normal texture here, so we can sample from it
		renderer->AddResourceToSlot(10, vctgi.GetVoxelTexture(), true, PipelineStage::COMPUTE, true);		// Add all the mips except the first so they can be used as an array in the shader
	}

	void RenderingPath::Dispose()
	{
		if (quadMesh.vao)
			delete quadMesh.vao;

		if (mainLightUBO)
			delete mainLightUBO;
		if (frameUBO)
			delete frameUBO;

		frameGraph.Dispose();
		font.Dispose();
		tod.Dispose();
		volumetricClouds.Dispose();
		projectedGridWater.Dispose();
		vctgi.Dispose();
	}

	void RenderingPath::Resize(unsigned int width, unsigned int  height)
	{
		this->width = width;
		this->height = height;

		frameGraph.GetPass("reflection").Resize(width / 2, height / 2);
		frameGraph.GetPass("refraction").Resize(width / 2, height / 2);
		frameGraph.GetPass("base").Resize(width, height);
		frameGraph.GetPass("BrightPass").Resize(width / 2, height / 2);
		frameGraph.GetPass("DownsamplePass4").Resize(width / 4, height / 4);
		frameGraph.GetPass("DownsamplePass8").Resize(width / 8, height / 8);
		frameGraph.GetPass("DownsamplePass16").Resize(width / 16, height / 16);
		frameGraph.GetPass("Upsample8").Resize(width / 8, height / 8);
		frameGraph.GetPass("Upsample4").Resize(width / 4, height / 4);
		//frameGraph.GetPass("postProcessPass").Resize(width, height);
#ifdef EDITOR
		//frameGraph.GetPass("fxaa").Resize(width, height);
		frameGraph.GetPass("postProcessPass").Resize(width, height);
#endif

		uiCamera.SetProjectionMatrix(0.0f, (float)width, 0.0f, (float)height, 0.0f, 10.0f);
	}

	void RenderingPath::Render()
	{
		tod.Update(game->GetDeltaTime());
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

		CascadedShadowMap::Update(csmInfo, *mainCamera, mainLightDir);
		
		glm::vec3 fakeLightPos = mainCamera->GetPosition() + mainLightDir * 100.0f;
		glm::vec4 lightScreenPos = mainCamera->GetProjectionMatrix() * mainCamera->GetViewMatrix() * glm::vec4(fakeLightPos, 1.0f);
		lightScreenPos.x /= lightScreenPos.w;
		lightScreenPos.y /= lightScreenPos.w;
		lightScreenPos.z /= lightScreenPos.w;

		frameData.lightScreenPos = glm::vec2((lightScreenPos.x + 1.0f) / 2.0f, (lightScreenPos.y + 1.0f) / 2.0f);

		float lightShaftsIntensity = 1.0f - sqrtf(frameData.lightScreenPos.x * frameData.lightScreenPos.x + frameData.lightScreenPos.y * frameData.lightScreenPos.y) * 0.15f;
		lightShaftsIntensity = std::min(1.0f, lightShaftsIntensity);
		lightShaftsIntensity = std::max(0.0f, lightShaftsIntensity);

		if (lightScreenPos.z < 0.0f || lightScreenPos.z > 1.0f)
			lightShaftsIntensity = 0.0f;
		else
			lightShaftsIntensity = 1.0f * (lightShaftsIntensity * lightShaftsIntensity * lightShaftsIntensity);

		
		// Update the lighting ubo
		DirLightUBO ubo = {};

		if (lockMainLightToTOD)
		{	
			ubo.dirAndIntensity = glm::vec4(mainLightDir, t.intensity);
			ubo.dirLightColor = glm::vec4(t.dirLightColor, t.ambient);
			ubo.skyColor = t.skyColor;

			frameData.lightShaftsIntensity = lightShaftsIntensity * t.lightShaftsIntensity * 0.5f;
			frameData.fogParams.y = t.heightFogDensity;
			frameData.fogInscatteringColor = glm::vec4(t.fogInscatteringColor, 0.0f);
			frameData.lightInscatteringColor = glm::vec4(t.lightInscaterringColor, 0.0f);
		}
		else
		{
			ubo.dirAndIntensity = glm::vec4(mainDirectionalLight.direction, mainDirectionalLight.intensity);
			ubo.dirLightColor = glm::vec4(mainDirectionalLight.color, mainDirectionalLight.ambient);
			ubo.skyColor = glm::vec4(0.0f);

			frameData.lightShaftsIntensity = baseLightShaftsIntensity * lightShaftsIntensity;
		}

		//ubo.cascadeEnd = glm::vec4(csmInfo.cascadeSplitEnd[0], csmInfo.cascadeSplitEnd[1], csmInfo.cascadeSplitEnd[2], csmInfo.cascadeSplitEnd[3]);
		ubo.cascadeEnd = glm::vec4(csmInfo.cascadeSplitEnd[0], csmInfo.cascadeSplitEnd[1], csmInfo.cascadeSplitEnd[2], 0.0f);
		ubo.lightSpaceMatrix[0] = csmInfo.viewProjLightSpace[0];
		ubo.lightSpaceMatrix[1] = csmInfo.viewProjLightSpace[1];
		ubo.lightSpaceMatrix[2] = csmInfo.viewProjLightSpace[2];
		//ubo.lightSpaceMatrix[3] = csmInfo.viewProjLightSpace[3];

		if (renderer->GetCurrentAPI() == GraphicsAPI::D3D11)
		{
			ubo.lightSpaceMatrix[0] = glm::transpose(ubo.lightSpaceMatrix[0]);
			ubo.lightSpaceMatrix[1] = glm::transpose(ubo.lightSpaceMatrix[1]);
			ubo.lightSpaceMatrix[2] = glm::transpose(ubo.lightSpaceMatrix[2]);
			//ubo.lightSpaceMatrix[3] = glm::transpose(ubo.lightSpaceMatrix[3]);
		}
		mainLightUBO->Update(&ubo, sizeof(DirLightUBO), 0);


		vctgi.UpdateProjection(mainCamera->GetPosition());
		const VoxelizationData &data = vctgi.GetVoxelizationData();

		frameData.orthoProjX = data.orthoProjX;
		frameData.orthoProjY = data.orthoProjY;
		frameData.orthoProjZ = data.orthoProjZ;
		frameData.timeElapsed = game->GetTimeElapsed();
		frameData.voxelGridSize = vctgi.GetVoxelGridSize();
		frameData.voxelScale = vctgi.GetVoxelScale();

		frameData.timeOfDay = tod.GetCurrentTime();

		const VolumetricCloudsData &vcd = volumetricClouds.GetVolumetricCloudsData();
		frameData.ambientBottomColor = vcd.ambientBottomColor;
		frameData.ambientTopColor = vcd.ambientTopColor;
		frameData.ambientMult = vcd.ambientMult;
		frameData.cloudCoverage = vcd.cloudCoverage;
		frameData.cloudStartHeight = vcd.cloudStartHeight;
		frameData.cloudLayerThickness = vcd.cloudLayerThickness;
		frameData.cloudLayerTopHeight = vcd.cloudLayerTopHeight;
		frameData.densityMult = vcd.densityMult;
		frameData.detailScale = vcd.detailScale;
		frameData.directLightMult = vcd.directLightMult;
		frameData.forwardSilverLiningIntensity = vcd.forwardSilverLiningIntensity;
		frameData.hgForward = vcd.hgForward;
		frameData.highCloudsCoverage = vcd.highCloudsCoverage;
		frameData.highCloudsTimeScale = vcd.highCloudsTimeScale;
		frameData.silverLiningIntensity = vcd.silverLiningIntensity;
		frameData.silverLiningSpread = vcd.silverLiningSpread;
		frameData.timeScale = vcd.timeScale;
		frameData.frameNumber = volumetricClouds.GetFrameNumber();
		frameData.cloudUpdateBlockSize = volumetricClouds.GetUpdateBlockSize();
		frameData.cloudsInvProjJitter = glm::inverse(mainCamera->GetProjectionMatrix()) * volumetricClouds.GetJitterMatrix();

		glm::vec2 windDirStr;
		windDirStr.x = glm::sin(game->GetTimeElapsed()) * 0.5f;
		windDirStr.y = glm::cos(game->GetTimeElapsed()) * 0.5f;

		frameData.windDirStr = windDirStr;

		projectedGridWater.Update(mainCamera, game->GetDeltaTime());		// Make sure to update the grid before updating the buffer otherwise the shader will get old values and will cause problems at the edge of the image when rotating the camera
		frameData.projGridViewFrame = projectedGridWater.GetViewFrame();
		frameData.viewCorner0 = projectedGridWater.GetViewCorner0();
		frameData.viewCorner1 = projectedGridWater.GetViewCorner1();
		frameData.viewCorner2 = projectedGridWater.GetViewCorner2();
		frameData.viewCorner3 = projectedGridWater.GetViewCorner3();
		frameData.waterNormalMapOffset = projectedGridWater.GetNormalMapOffset();

		frameData.screenRes = glm::vec2((float)width, (float)height);
		//frameData.invScreenRes = glm::vec2(1.0f / width, 1.0f / height);
	
		Terrain *terrain = game->GetTerrain();
		if (terrain)
		{
			const glm::vec3 &p = terrain->GetIntersectionPoint();
			frameData.terrainEditParams = glm::vec4(p.x, p.z, terrain->IsBeingEdited() ? 1.0f : 0.0f, terrain->GetBrushRadius());
			frameData.terrainEditParams2 = glm::vec4(terrain->GetBrushStrength(), 0.0f, 0.0f, 0.0f);
		}

		frameData.deltaTime = game->GetDeltaTime();

		frameUBO->Update(&frameData, sizeof(FrameUBO), 0);
	}

	void RenderingPath::EnableTerrainEditing()
	{
		SetupTerrainEditPass();
		isTerrainEditingEnabled = true;

		frameGraph.Bake(renderer);
		frameGraph.Setup();
	}

	void RenderingPath::DisableTerrainEditing()
	{
		//frameGraph.RemovePass("terrainEdit");
		isTerrainEditingEnabled = false;
	}

	void RenderingPath::SetMainCamera(Camera *camera)
	{
		if (!camera)
			return;

		mainCamera = camera;
		volumetricClouds.SetCamera(camera);
	}

	void RenderingPath::SaveRenderingSettings(const std::string &path)
	{
		Serializer s(game->GetFileManager());
		s.OpenForWriting();

		s.Write(frameData.giIntensity);
		s.Write(frameData.skyColorMultiplier);
		s.Write(frameData.aoIntensity);
		s.Write(frameData.voxelGridSize);
		s.Write(frameData.voxelScale);
		s.Write(frameData.enableGI);
		s.Write(frameData.bloomIntensity);
		s.Write(frameData.bloomThreshold);
		s.Write(frameData.lightShaftsParams);
		s.Write(frameData.lightShaftsColor);
		s.Write(frameData.fogParams);
		s.Write(frameData.fogInscatteringColor);
		s.Write(frameData.lightInscatteringColor);
		s.Write(frameData.cloudCoverage);
		s.Write(frameData.cloudStartHeight);
		s.Write(frameData.cloudLayerThickness);
		s.Write(frameData.cloudLayerTopHeight);
		s.Write(frameData.timeScale);
		s.Write(frameData.hgForward);
		s.Write(frameData.densityMult);
		s.Write(frameData.ambientMult);
		s.Write(frameData.directLightMult);
		s.Write(frameData.detailScale);
		s.Write(frameData.highCloudsCoverage);
		s.Write(frameData.highCloudsTimeScale);
		s.Write(frameData.silverLiningIntensity);
		s.Write(frameData.silverLiningSpread);
		s.Write(frameData.forwardSilverLiningIntensity);
		s.Write(baseLightShaftsIntensity);
		s.Write(frameData.ambientTopColor);
		s.Write(frameData.ambientBottomColor);

		s.Write(mainDirectionalLight.direction);
		s.Write(mainDirectionalLight.color);
		s.Write(mainDirectionalLight.intensity);
		s.Write(mainDirectionalLight.ambient);

		s.Write(lockMainLightToTOD);

		s.Write(projectedGridWater.GetWaterHeight());

		s.Write(tod.GetCurrentMonth());
		s.Write(tod.GetCurrentDay());
		s.Write(tod.GetCurrentTime());

		s.Save(path);
		s.Close();
	}

	void RenderingPath::LoadRenderingSettings(const std::string &path)
	{
		Serializer s(game->GetFileManager());
		s.OpenForReading(path);

		if (s.IsOpen())
		{
			s.Read(frameData.giIntensity);
			s.Read(frameData.skyColorMultiplier);
			s.Read(frameData.aoIntensity);
			s.Read(frameData.voxelGridSize);
			s.Read(frameData.voxelScale);
			s.Read(frameData.enableGI);
			s.Read(frameData.bloomIntensity);
			s.Read(frameData.bloomThreshold);
			s.Read(frameData.lightShaftsParams);
			s.Read(frameData.lightShaftsColor);
			s.Read(frameData.fogParams);
			s.Read(frameData.fogInscatteringColor);
			s.Read(frameData.lightInscatteringColor);
			s.Read(frameData.cloudCoverage);
			s.Read(frameData.cloudStartHeight);
			s.Read(frameData.cloudLayerThickness);
			s.Read(frameData.cloudLayerTopHeight);
			s.Read(frameData.timeScale);
			s.Read(frameData.hgForward);
			s.Read(frameData.densityMult);
			s.Read(frameData.ambientMult);
			s.Read(frameData.directLightMult);
			s.Read(frameData.detailScale);
			s.Read(frameData.highCloudsCoverage);
			s.Read(frameData.highCloudsTimeScale);
			s.Read(frameData.silverLiningIntensity);
			s.Read(frameData.silverLiningSpread);
			s.Read(frameData.forwardSilverLiningIntensity);
			s.Read(baseLightShaftsIntensity);
			s.Read(frameData.ambientTopColor);
			s.Read(frameData.ambientBottomColor);

			s.Read(mainDirectionalLight.direction);
			s.Read(mainDirectionalLight.color);
			s.Read(mainDirectionalLight.intensity);
			s.Read(mainDirectionalLight.ambient);

			s.Read(lockMainLightToTOD);

			float waterHeight = 0.0f;
			s.Read(waterHeight);
			projectedGridWater.SetWaterHeight(waterHeight);

			int day, month;
			float curTime;
			s.Read(month);
			s.Read(day);
			s.Read(curTime);

			tod.SetCurrentMonth(month);
			tod.SetCurrentDay(day);
			tod.SetCurrentTime(curTime);
		}

		s.Close();
	}

	void RenderingPath::SetupCSMPass()
	{
		// Cascaded shadow map pass
		csmInfo.cascadeSplitEnd[0] = 0.02f;
		csmInfo.cascadeSplitEnd[1] = 0.1f;
		csmInfo.cascadeSplitEnd[2] = 0.45f;
		//csmInfo.cascadeSplitEnd[3] = 0.7f;

		Pass &csmPass = frameGraph.AddPass("csm");
		AttachmentInfo shadowMap = {};
		shadowMap.width = SHADOW_MAP_RES * CASCADE_COUNT;
		shadowMap.height = SHADOW_MAP_RES;
		shadowMap.params = { TextureWrap::CLAMP_TO_BORDER, TextureFilter::LINEAR, TextureFormat::DEPTH_COMPONENT, TextureInternalFormat::DEPTH_COMPONENT24, TextureDataType::FLOAT, false, true };
		csmPass.AddDepthOutput("shadowMap", shadowMap);

		csmPass.SetOnSetup([this](const Pass *thisPass)
		{
			//debugMatInstance->textures[0] = thisPass->GetFramebuffer()->GetDepthTexture();
			csmFB = thisPass->GetFramebuffer();
		});

		csmPass.SetOnExecute([this]() {PerformCSMPass(); });
	}

	void RenderingPath::SetupBloomPasses()
	{
		Pass &brightPass = frameGraph.AddPass("BrightPass");

		// Bright pass is done at half res
		AttachmentInfo brightPassAttachment = {};
		brightPassAttachment.params = { TextureWrap::CLAMP_TO_EDGE, TextureFilter::LINEAR, TextureFormat::RGBA, TextureInternalFormat::RGBA16F, TextureDataType::FLOAT, false, false };
		brightPassAttachment.width = width / 2;
		brightPassAttachment.height = height / 2;
		brightPassAttachment.initialState = InitialState::CLEAR;

		brightPass.AddTextureInput("color");
		brightPass.AddTextureOutput("brightPass", brightPassAttachment);

		brightPass.SetOnSetup([this](const Pass *thisPass)
		{
			brightPassFB = thisPass->GetFramebuffer();
			brightPassMatInstance = renderer->CreateMaterialInstanceFromBaseMat(this->game->GetScriptManager(), "Data/Resources/Materials/brightpass_mat.lua", quadMesh.vao->GetVertexInputDescs());
			brightPassMatInstance->textures[0] = hdrFB->GetColorTextureByIndex(0);
			renderer->UpdateMaterialInstance(brightPassMatInstance);
		});

		brightPass.SetOnResized([this](const Pass *thisPass)
		{
			brightPassMatInstance->textures[0] = hdrFB->GetColorTextureByIndex(0);
		});

		brightPass.SetOnExecute([this]() { PerformBrightPass(); });

		Pass &downsample4Pass = frameGraph.AddPass("DownsamplePass4");
		// Downsample 4 is done at quarter res
		AttachmentInfo downsample4Attach = {};
		downsample4Attach.params = { TextureWrap::CLAMP_TO_EDGE, TextureFilter::LINEAR, TextureFormat::RGBA, TextureInternalFormat::RGBA16F, TextureDataType::FLOAT, false, false };
		downsample4Attach.width = width / 4;
		downsample4Attach.height = height / 4;
		downsample4Attach.initialState = InitialState::CLEAR;

		downsample4Pass.AddTextureInput("brightPass");
		downsample4Pass.AddTextureOutput("downsample4", downsample4Attach);

		downsample4Pass.SetOnSetup([this](const Pass *thisPass)
		{
			downsampleFB[0] = thisPass->GetFramebuffer();
			downsample4MatInstance = renderer->CreateMaterialInstanceFromBaseMat(this->game->GetScriptManager(), "Data/Resources/Materials/downsample_mat.lua", quadMesh.vao->GetVertexInputDescs());
			downsample4PassID = downsample4MatInstance->baseMaterial->GetShaderPassIndex("DownsamplePass4");
			downsample4MatInstance->textures[0] = brightPassFB->GetColorTexture();
			renderer->UpdateMaterialInstance(downsample4MatInstance);
		});

		downsample4Pass.SetOnResized([this](const Pass *thisPass)
		{
			downsample4MatInstance->textures[0] = brightPassFB->GetColorTexture();
		});

		downsample4Pass.SetOnExecute([this]() { PerformDownsample4Pass(); });

		Pass &downsample8Pass = frameGraph.AddPass("DownsamplePass8");
		// Downsample 8 is done at 1/8 res
		AttachmentInfo downsample8Attach = {};
		downsample8Attach.params = { TextureWrap::CLAMP_TO_EDGE, TextureFilter::LINEAR, TextureFormat::RGBA, TextureInternalFormat::RGBA16F, TextureDataType::FLOAT, false, false };
		downsample8Attach.width = width / 8;
		downsample8Attach.height = height / 8;
		downsample8Attach.initialState = InitialState::CLEAR;

		downsample8Pass.AddTextureInput("downsample4");
		downsample8Pass.AddTextureOutput("downsample8", downsample8Attach);

		downsample8Pass.SetOnSetup([this](const Pass *thisPass)
		{
			downsampleFB[1] = thisPass->GetFramebuffer();
			downsample8MatInstance = renderer->CreateMaterialInstanceFromBaseMat(this->game->GetScriptManager(), "Data/Resources/Materials/downsample_mat.lua", quadMesh.vao->GetVertexInputDescs());
			downsample8PassID = downsample8MatInstance->baseMaterial->GetShaderPassIndex("DownsamplePass8");
			downsample8MatInstance->textures[0] = downsampleFB[0]->GetColorTexture();
			renderer->UpdateMaterialInstance(downsample8MatInstance);
		});

		downsample8Pass.SetOnResized([this](const Pass *thisPass)
		{
			downsample8MatInstance->textures[0] = downsampleFB[0]->GetColorTexture();
		});

		downsample8Pass.SetOnExecute([this]() { PerformDownsample8Pass(); });

		// Downsample 16 is done at 1/16 res
		Pass &downsample16Pass = frameGraph.AddPass("DownsamplePass16");

		AttachmentInfo downsample16Attach = {};
		downsample16Attach.params = { TextureWrap::CLAMP_TO_EDGE, TextureFilter::LINEAR, TextureFormat::RGBA, TextureInternalFormat::RGBA16F, TextureDataType::FLOAT, false, false };
		downsample16Attach.width = width / 16;
		downsample16Attach.height = height / 16;
		downsample16Attach.initialState = InitialState::CLEAR;

		downsample16Pass.AddTextureInput("downsample8");
		downsample16Pass.AddTextureOutput("downsample16", downsample16Attach);

		downsample16Pass.SetOnSetup([this](const Pass *thisPass)
		{
			downsampleFB[2] = thisPass->GetFramebuffer();
			downsample16MatInstance = renderer->CreateMaterialInstanceFromBaseMat(this->game->GetScriptManager(), "Data/Resources/Materials/downsample_mat.lua", quadMesh.vao->GetVertexInputDescs());
			downsample16PassID = downsample16MatInstance->baseMaterial->GetShaderPassIndex("DownsamplePass16");
			downsample16MatInstance->textures[0] = downsampleFB[1]->GetColorTexture();
			renderer->UpdateMaterialInstance(downsample16MatInstance);
		});

		downsample16Pass.SetOnResized([this](const Pass *thisPass)
		{
			downsample16MatInstance->textures[0] = downsampleFB[1]->GetColorTexture();
		});

		downsample16Pass.SetOnExecute([this]() { PerformDownsample16Pass(); });

		// Upsample from 1/16 to 1/8
		Pass &upsample8Pass = frameGraph.AddPass("Upsample8");

		AttachmentInfo upsample8Attach = {};
		upsample8Attach.params = { TextureWrap::CLAMP_TO_EDGE, TextureFilter::LINEAR, TextureFormat::RGBA, TextureInternalFormat::RGBA16F, TextureDataType::FLOAT, false, false };
		upsample8Attach.width = width / 8;
		upsample8Attach.height = height / 8;
		upsample8Attach.initialState = InitialState::CLEAR;

		upsample8Pass.AddTextureInput("downsample16");
		upsample8Pass.AddTextureInput("downsample8");
		upsample8Pass.AddTextureOutput("upsample8", upsample8Attach);

		upsample8Pass.SetOnSetup([this](const Pass *thisPass)
		{
			upsampleFB[0] = thisPass->GetFramebuffer();
			upsample8MatInstance = renderer->CreateMaterialInstanceFromBaseMat(this->game->GetScriptManager(), "Data/Resources/Materials/upsample_mat.lua", quadMesh.vao->GetVertexInputDescs());
			upsample8PassID = upsample8MatInstance->baseMaterial->GetShaderPassIndex("Upsample8");
			upsample8MatInstance->textures[0] = downsampleFB[2]->GetColorTexture();
			upsample8MatInstance->textures[1] = downsampleFB[1]->GetColorTexture();
			renderer->UpdateMaterialInstance(upsample8MatInstance);
		});

		upsample8Pass.SetOnResized([this](const Pass *thisPass)
		{
			upsample8MatInstance->textures[0] = downsampleFB[2]->GetColorTexture();
			upsample8MatInstance->textures[1] = downsampleFB[1]->GetColorTexture();
		});

		upsample8Pass.SetOnExecute([this]() { PerformUpsample8Pass(); });

		// Upsample from 1/8 to 1/4
		Pass &upsample4Pass = frameGraph.AddPass("Upsample4");

		AttachmentInfo upsample4Attach = {};
		upsample4Attach.params = { TextureWrap::CLAMP_TO_EDGE, TextureFilter::LINEAR, TextureFormat::RGBA, TextureInternalFormat::RGBA16F, TextureDataType::FLOAT, false, false };
		upsample4Attach.width = width / 4;
		upsample4Attach.height = height / 4;
		upsample4Attach.initialState = InitialState::CLEAR;

		upsample4Pass.AddTextureInput("upsample8");
		upsample4Pass.AddTextureInput("downsample4");
		upsample4Pass.AddTextureOutput("upsample4", upsample4Attach);

		upsample4Pass.SetOnSetup([this](const Pass *thisPass)
		{
			upsampleFB[1] = thisPass->GetFramebuffer();
			upsample4MatInstance = renderer->CreateMaterialInstanceFromBaseMat(this->game->GetScriptManager(), "Data/Resources/Materials/upsample_mat.lua", quadMesh.vao->GetVertexInputDescs());
			upsample4PassID = upsample4MatInstance->baseMaterial->GetShaderPassIndex("Upsample4");
			upsample4MatInstance->textures[0] = upsampleFB[0]->GetColorTexture();
			upsample4MatInstance->textures[1] = downsampleFB[0]->GetColorTexture();
			renderer->UpdateMaterialInstance(upsample4MatInstance);
		});

		upsample4Pass.SetOnResized([this](const Pass *thisPass)
		{
			upsample4MatInstance->textures[0] = upsampleFB[0]->GetColorTexture();
			upsample4MatInstance->textures[1] = downsampleFB[0]->GetColorTexture();
		});

		upsample4Pass.SetOnExecute([this]() { PerformUpsample4Pass(); });
	}

	void RenderingPath::SetupReflectionPass()
	{
		Pass &reflectionPass = frameGraph.AddPass("reflection");

		// If the attachments are not equal to the hdr pass then the render passes are not compatible and will cause an error. Add handling for that?
		AttachmentInfo reflectionAttach = {};
		reflectionAttach.width = width / 2;
		reflectionAttach.height = height / 2;
		reflectionAttach.params = { TextureWrap::CLAMP_TO_EDGE, TextureFilter::LINEAR, TextureFormat::RGBA, TextureInternalFormat::RGBA16F, TextureDataType::FLOAT, false, false };
		AttachmentInfo depthAttachment = {};
		depthAttachment.params = { TextureWrap::CLAMP_TO_EDGE, TextureFilter::NEAREST, TextureFormat::DEPTH_COMPONENT, TextureInternalFormat::DEPTH_COMPONENT24, TextureDataType::FLOAT, false, false };
		depthAttachment.width = width / 2;
		depthAttachment.height = height / 2;


		//reflectionPass.AddDepthInput("shadowMap");			// Right we're only rendering the skydome for reflection so we don't need the shadow map
		reflectionPass.AddTextureOutput("reflectionTex", reflectionAttach);
		reflectionPass.AddDepthOutput("reflectionDepth", depthAttachment);

		reflectionPass.SetOnSetup([this](const Pass *thisPass)
		{
			reflectionFB = thisPass->GetFramebuffer();
			projectedGridWater.GetMaterialInstance()->textures[0] = thisPass->GetFramebuffer()->GetColorTexture();
		});
		reflectionPass.SetOnResized([this](const Pass *thisPass)
		{
			projectedGridWater.GetMaterialInstance()->textures[0] = thisPass->GetFramebuffer()->GetColorTexture();
			renderer->UpdateMaterialInstance(projectedGridWater.GetMaterialInstance());
		});
		reflectionPass.SetOnExecute([this]() {PerformReflectionPass(); });
	}

	void RenderingPath::SetupRefractionPass()
	{
		Pass &refractionPass = frameGraph.AddPass("refraction");
		// If the attachments are not equal to the hdr pass then the render passes are not compatible and will cause an error. Add handling for that?
		AttachmentInfo refractionColorAttach = {};
		refractionColorAttach.width = width / 2;
		refractionColorAttach.height = height / 2;
		refractionColorAttach.params = { TextureWrap::CLAMP_TO_EDGE, TextureFilter::LINEAR, TextureFormat::RGBA, TextureInternalFormat::RGBA16F, TextureDataType::FLOAT, false, false };
		AttachmentInfo refractionDepthAttachment = {};
		refractionDepthAttachment.params = { TextureWrap::CLAMP_TO_EDGE, TextureFilter::NEAREST, TextureFormat::DEPTH_COMPONENT, TextureInternalFormat::DEPTH_COMPONENT24, TextureDataType::FLOAT, false, false };
		refractionDepthAttachment.width = width / 2;
		refractionDepthAttachment.height = height / 2;

		refractionPass.AddDepthInput("shadowMap");
		refractionPass.AddTextureOutput("refractionTex", refractionColorAttach);
		refractionPass.AddDepthOutput("refractionDepth", refractionDepthAttachment);

		refractionPass.SetOnSetup([this](const Pass *thisPass)
		{
			refractionFB = thisPass->GetFramebuffer();
			// Projected grid water
			projectedGridWater.Init(renderer, game->GetScriptManager(), 4.0f);
			projectedGridWater.GetMaterialInstance()->textures[2] = thisPass->GetFramebuffer()->GetColorTexture();
			projectedGridWater.GetMaterialInstance()->textures[3] = thisPass->GetFramebuffer()->GetDepthTexture();
			renderer->UpdateMaterialInstance(projectedGridWater.GetMaterialInstance());
		});
		refractionPass.SetOnResized([this](const Pass *thisPass)
		{
			projectedGridWater.GetMaterialInstance()->textures[2] = thisPass->GetFramebuffer()->GetColorTexture();
			projectedGridWater.GetMaterialInstance()->textures[3] = thisPass->GetFramebuffer()->GetDepthTexture();
			renderer->UpdateMaterialInstance(projectedGridWater.GetMaterialInstance());
		});
		refractionPass.SetOnExecute([this]() {PerformRefractionPass(); });
	}

	void RenderingPath::SetupVoxelizationPass()
	{
		Pass &voxelizationPass = frameGraph.AddPass("voxelization");
		voxelizationPass.DisableWritesToFramebuffer();
		voxelizationPass.AddImageOutput("voxelTexture", vctgi.GetVoxelTexture(), true);
		voxelizationPass.AddDepthInput("shadowMap");

		voxelizationPass.SetOnBarriers([this]()
		{
			// Make sure the reads to the voxel texture are finished and transition it from read to write
			BarrierImage bi = {};
			bi.image = vctgi.GetVoxelTexture();
			bi.readToWrite = true;
			bi.baseMip = 0;
			bi.numMips = vctgi.GetVoxelTexture()->GetMipLevels();
			//bi.transitionToGeneral = true;

			Barrier b = {};
			b.images.push_back(bi);
			b.srcStage = PipelineStage::FRAGMENT;
			b.dstStage = PipelineStage::FRAGMENT;

			renderer->PerformBarrier(b);
		});

		voxelizationPass.SetOnExecute([this]()
		{
			vctgi.Voxelize(renderQueues[3]);
		});
	}

	void RenderingPath::SetupFXAAPass()
	{
		Pass &p = frameGraph.AddPass("fxaa");
		p.AddTextureInput("postProcessOutput");

#ifdef EDITOR
		Engine::AttachmentInfo finalAttach = {};
		finalAttach.width = game->GetRenderer()->GetWidth();
		finalAttach.height = game->GetRenderer()->GetHeight();
		finalAttach.params = { Engine::TextureWrap::CLAMP_TO_EDGE, Engine::TextureFilter::LINEAR, Engine::TextureFormat::RGBA, Engine::TextureInternalFormat::RGBA8, Engine::TextureDataType::UNSIGNED_BYTE, false, false };

		p.AddTextureOutput("final", finalAttach);
#endif

		p.SetOnSetup([this](const Pass *thisPass)
		{
			fxaaFB = thisPass->GetFramebuffer();
			fxaaMat = renderer->CreateMaterialInstanceFromBaseMat(game->GetScriptManager(), "Data/Resources/Materials/fxaa_mat.lua", quadMesh.vao->GetVertexInputDescs());
			fxaaMat->textures[0] = postProcessFB->GetColorTexture();
			renderer->UpdateMaterialInstance(fxaaMat);
		});
		p.SetOnResized([this](const Pass *thisPass)
		{
			fxaaMat->textures[0] = postProcessFB->GetColorTexture();
			renderer->UpdateMaterialInstance(fxaaMat);
		});

		p.SetOnExecute([this]()
		{
			RenderItem ri = {};
			ri.mesh = &quadMesh;
			ri.matInstance = fxaaMat;

			renderer->Submit(ri);
		});
	}

	void RenderingPath::SetupTerrainEditPass()
	{
		Pass &p = frameGraph.AddPass("terrainEdit");
		p.SetIsCompute(true);
		p.AddImageOutput("terrainImg", game->GetTerrain()->GetMaterialInstance()->textures[0]);

		p.SetOnSetup([this](const Pass *thisPass)
		{
			terrainEditMat = renderer->CreateMaterialInstanceFromBaseMat(game->GetScriptManager(), "Data/Resources/Materials/terrain_edit_mat.lua", {});
			terrainEditMat->textures[0] = game->GetTerrain()->GetMaterialInstance()->textures[0];
			renderer->UpdateMaterialInstance(terrainEditMat);
		});

		p.SetOnBarriers([this]()
		{
			// Make sure the image has been read
			BarrierImage bi = {};
			bi.image = game->GetTerrain()->GetMaterialInstance()->textures[0];
			bi.readToWrite = true;
			bi.baseMip = 0;
			bi.numMips = 1;

			Barrier b = {};
			b.images.push_back(bi);
			b.srcStage = PipelineStage::VERTEX;
			b.dstStage = PipelineStage::COMPUTE;

			renderer->PerformBarrier(b);
		});

		p.SetOnExecute([this]()
		{
			Texture *t = game->GetTerrain()->GetMaterialInstance()->textures[0];

			DispatchItem item = {};
			item.numGroupsX = t->GetWidth() / 16;
			item.numGroupsY = t->GetHeight() / 16;
			item.numGroupsZ = 1;
			item.matInstance = terrainEditMat;
			item.shaderPass = 0;

			renderer->Dispatch(item);
		});
	}

	void RenderingPath::PerformCSMPass()
	{
		for (size_t i = 0; i < CASCADE_COUNT; i++)
		{
			Viewport viewport;
			viewport.x = i * SHADOW_MAP_RES;
			viewport.y = 0;
			viewport.width = SHADOW_MAP_RES;
			viewport.height = SHADOW_MAP_RES;
			if (renderQueues[i].size() > 0)
			{
				renderer->SetViewport(viewport);
				renderer->SetCamera(&csmInfo.cameras[i]);
				renderer->Submit(renderQueues[i]);
			}
		}
	}

	void RenderingPath::PerformBrightPass()
	{
		RenderItem ri = {};
		ri.mesh = &quadMesh;
		ri.shaderPass = 0;
		ri.matInstance = brightPassMatInstance;

		renderer->Submit(ri);
	}

	void RenderingPath::PerformDownsample4Pass()
	{
		// Downsample to 1/4
		RenderItem ri = {};
		ri.mesh = &quadMesh;
		ri.shaderPass = downsample4PassID;
		ri.matInstance = downsample4MatInstance;

		renderer->Submit(ri);
	}

	void RenderingPath::PerformDownsample8Pass()
	{
		// Downsample to 1/8
		RenderItem ri = {};
		ri.mesh = &quadMesh;
		ri.shaderPass = downsample8PassID;
		ri.matInstance = downsample8MatInstance;

		renderer->Submit(ri);
	}

	void RenderingPath::PerformDownsample16Pass()
	{
		// Downsample to 1/16
		RenderItem ri = {};
		ri.mesh = &quadMesh;
		ri.shaderPass = downsample16PassID;
		ri.matInstance = downsample16MatInstance;

		renderer->Submit(ri);
	}

	void RenderingPath::PerformUpsample8Pass()
	{
		// Upsample to 1/8
		RenderItem ri = {};
		ri.mesh = &quadMesh;
		ri.shaderPass = upsample8PassID;
		ri.matInstance = upsample8MatInstance;

		renderer->Submit(ri);
	}

	void RenderingPath::PerformUpsample4Pass()
	{
		// Upsample to 1/4
		RenderItem ri = {};
		ri.mesh = &quadMesh;
		ri.shaderPass = upsample4PassID;
		ri.matInstance = upsample4MatInstance;

		//renderer->Render(ri, &ppd.sampleScale, sizeof(ppd.sampleScale));
		renderer->Submit(ri);
	}

	void RenderingPath::PerformReflectionPass()
	{
		glm::vec3 camPos = mainCamera->GetPosition();
		float distance = 2.0f * (camPos.y - projectedGridWater.GetWaterHeight());
		camPos.y -= distance;
		mainCamera->SetPosition(camPos);
		float pitch = mainCamera->GetPitch();
		mainCamera->SetPitch(-pitch);		// Invert the pitch

		renderer->SetCamera(mainCamera/*, glm::vec4(0.0f, 12.0f + 0.5f, 0.0f, 1.0f)*/);

		//renderer->Submit(renderQueues[4]);
		tod.Render(renderer);

		// Reset the camera
		camPos.y += distance;
		mainCamera->SetPosition(camPos);
		mainCamera->SetPitch(pitch);
	}

	void RenderingPath::PerformRefractionPass()
	{
		renderer->SetCamera(mainCamera, glm::vec4(0.0f, -1.0f, 0.0f, projectedGridWater.GetWaterHeight() + 1.0f));

		if (game->GetTerrain() && game->GetTerrain()->IsVisible())
		{
			renderer->Submit(game->GetTerrain()->GetTerrainRenderItem());
		}
	}
}
