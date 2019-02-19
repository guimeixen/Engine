#include "RenderingWindow.h"

#include "Game\Game.h"
#include "EditorManager.h"
#include "Graphics\Renderer.h"
#include "Graphics\Effects\MainView.h"
#include "Program\Utils.h"

#include "include\glm\gtc\type_ptr.hpp"

#include "imgui\imgui.h"
#include "imgui\imgui_dock.h"

void RenderingWindow::Init(Engine::Game *game, EditorManager *editorManager)
{
	this->game = game;
	this->editorManager = editorManager;

	nearPlane = game->GetRenderingPath()->GetMainCamera()->GetNearPlane();
	farPlane = game->GetRenderingPath()->GetMainCamera()->GetFarPlane();
	lightShaftsIntensity = game->GetRenderingPath()->GetFrameData().lightShaftsIntensity;

	Engine::TimeOfDayManager &timeOfDayManager = game->GetRenderingPath()->GetTOD();

	worldTime = timeOfDayManager.GetCurrentTime();
	azimuthOffset = timeOfDayManager.GetAzimuthOffset();

	Engine::VCTGI &vctgi = game->GetRenderingPath()->GetVCTGI();
	voxelGridSize = vctgi.GetVoxelGridSize();

	waterHeight = game->GetRenderingPath()->GetProjectedGridWater().GetWaterHeight();

	debugTypesStr.push_back("CSM shadow map");
	debugTypesStr.push_back("Bright pass");
	debugTypesStr.push_back("Reflection");
	debugTypesStr.push_back("Voxel Texture");
	debugTypesStr.push_back("Refraction");

	month = 1;
	day = 1;
}

void RenderingWindow::Render()
{
	Engine::FrameUBO &frameUBO = game->GetRenderingPath()->GetFrameData();

	if (ImGui::BeginDock("Rendering Window", &showWindow))
	{
		Engine::RenderingPath *renderingPath = game->GetRenderingPath();
		Engine::DirLight &mainDirLight = renderingPath->GetMainDirLight();
		Engine::DebugSettings &debugSettings = renderingPath->GetDebugSettings();
		Engine::TimeOfDayManager &timeOfDayManager = renderingPath->GetTOD();
		Engine::VCTGI &vctgi = renderingPath->GetVCTGI();
		Engine::FrameUBO &frameUBO = renderingPath->GetFrameData();
		Engine::ProjectedGridWater &projectedGridWater = game->GetRenderingPath()->GetProjectedGridWater();
		Engine::VolumetricCloudsData &vcd = game->GetRenderingPath()->GetVolumetricClouds().GetVolumetricCloudsData();

		Engine::FPSCamera *mainCamera = static_cast<Engine::FPSCamera*>(game->GetRenderingPath()->GetMainCamera());
		nearPlane = mainCamera->GetNearPlane();
		farPlane = mainCamera->GetFarPlane();

		lightShaftsIntensity = renderingPath->GetBaseLightShaftsIntensity();

		if (editorManager->WasProjectJustLoaded())
		{
			waterHeight = projectedGridWater.GetWaterHeight();
			lockToTOD = renderingPath->IsMainLightLockedToTOD();
			month = timeOfDayManager.GetCurrentMonth();
			day = timeOfDayManager.GetCurrentDay();
			worldTime = timeOfDayManager.GetCurrentTime();
		}

		if (ImGui::Checkbox("Show UI", &showUI))
			game->GetRenderingPath()->EnableUI(showUI);

		if (ImGui::SliderFloat("Time", &worldTime, 0.0f, 24.0f, "%.2f"))
			timeOfDayManager.SetCurrentTime(worldTime);

		if (ImGui::SliderInt("Month", &month, 1, 12))
			timeOfDayManager.SetCurrentMonth(month);

		int maxDay;
		if (month == 2)
			maxDay = 28;
		else if (month == 4 || month == 6 || month == 9 || month == 11)
			maxDay = 30;
		else
			maxDay = 31;

		if (ImGui::SliderInt("Day", &day, 1, maxDay))
			timeOfDayManager.SetCurrentDay(day);

		if (ImGui::DragFloat("Azimuth offset", &azimuthOffset, 0.1f))
			timeOfDayManager.SetAzimuthOffset(azimuthOffset);

		if (ImGui::Checkbox("Lock to TOD", &lockToTOD))
			renderingPath->LockMainLightToTOD(lockToTOD);

		if (ImGui::CollapsingHeader("Main Directional Light", ImGuiTreeNodeFlags_DefaultOpen))
		{
			ImGui::ColorEdit3("Color", glm::value_ptr(mainDirLight.color), 0);
			ImGui::DragFloat("Intensity", &mainDirLight.intensity, 0.01f, 0.0f);
			ImGui::DragFloat3("Direction", glm::value_ptr(mainDirLight.direction), 0.01f, -1.0f, 1.0f);
			ImGui::DragFloat("Ambient", &mainDirLight.ambient, 0.001f, 0.0f);
			
			if (ImGui::DragFloat("Light shafts intensity", &lightShaftsIntensity, 0.01f, 0.0f))
				renderingPath->SetBaseLightShaftsIntensity(lightShaftsIntensity);
			
			ImGui::ColorEdit3("Light shafts color", glm::value_ptr(frameUBO.lightShaftsColor), 0);
			ImGui::DragFloat("Light shafts density", &frameUBO.lightShaftsParams.x, 0.01f, 0.0f);
			ImGui::DragFloat("Light shafts decay", &frameUBO.lightShaftsParams.y, 0.01f, 0.0f);
			ImGui::DragFloat("Light shafts weight", &frameUBO.lightShaftsParams.z, 0.01f, 0.0f);

			ImGui::DragFloat("Light shafts exposure", &frameUBO.lightShaftsParams.w, 0.001f, 0.0f);
			if (ImGui::Checkbox("Enable GI", &enableGI))
				frameUBO.enableGI = (bool)enableGI;

			ImGui::DragFloat("GI Intensity", &frameUBO.giIntensity, 0.01f, 0.0f);
			ImGui::DragFloat("AO Intensity", &frameUBO.aoIntensity, 0.01f, 0.0f);

			if (ImGui::DragFloat("Voxel grid size", &voxelGridSize, 0.1f, 0.0f))
				vctgi.SetVoxelGridSize(voxelGridSize);

			//ImGui::ColorEdit3("", &frameUBO.sk)
			ImGui::DragFloat("Sky color multiplier", &frameUBO.skyColorMultiplier, 0.01f, 0.0f);		
		}
		if (ImGui::CollapsingHeader("Main camera", ImGuiTreeNodeFlags_DefaultOpen))
		{
			if (ImGui::DragFloat("Near plane", &nearPlane, 0.1f, 0.1f))
				mainCamera->SetNearPlane(nearPlane);
			if (ImGui::DragFloat("Far plane", &farPlane, 0.1f, 0.1f))
				mainCamera->SetFarPlane(farPlane);
		}
		if (ImGui::CollapsingHeader("Bloom", ImGuiTreeNodeFlags_DefaultOpen))
		{
			ImGui::DragFloat("Bloom Intensity", &frameUBO.bloomIntensity, 0.01f, 0.0f);
			ImGui::DragFloat("Bloom Threshold", &frameUBO.bloomThreshold, 0.01f, 0.0f);
			//ImGui::DragFloat("Bloom Radius", &frameUBO.bloomRadius, 0.01f, 1.0f);
		}
		if (ImGui::CollapsingHeader("Vignette", ImGuiTreeNodeFlags_DefaultOpen))
		{
			ImGui::DragFloat("Vignette Intensity", &frameUBO.vignetteParams.x, 0.01f, 0.0f);
			ImGui::DragFloat("Vignette Falloff", &frameUBO.vignetteParams.y, 0.01f, 0.0f);
		}
		if (ImGui::CollapsingHeader("Height fog", ImGuiTreeNodeFlags_DefaultOpen))
		{
			ImGui::DragFloat("Height", &frameUBO.fogParams.x, 0.1f);
			ImGui::DragFloat("Density", &frameUBO.fogParams.y, 0.001f, 0.0f, 1.0f, "%.5f");
			ImGui::ColorEdit3("Fog inscattering color", &frameUBO.fogInscatteringColor.x, 0);
			ImGui::ColorEdit3("Light inscattering color", &frameUBO.lightInscatteringColor.x, 0);
		}
		if (ImGui::CollapsingHeader("Volumetric clouds", ImGuiTreeNodeFlags_DefaultOpen))
		{
			ImGui::DragFloat("Cloud coverage", &vcd.cloudCoverage, 0.01f);

			if (ImGui::DragFloat("Cloud layer start height", &vcd.cloudStartHeight, 0.01f))
				vcd.cloudLayerThickness = vcd.cloudStartHeight + vcd.cloudLayerTopHeight;
			if (ImGui::DragFloat("Cloud layer top height", &vcd.cloudLayerTopHeight, 0.01f))
				vcd.cloudLayerThickness = vcd.cloudStartHeight + vcd.cloudLayerTopHeight;

			ImGui::SliderFloat("Detail scale", &vcd.detailScale, 0.0f, 16.0f);

			ImGui::DragFloat("Time scale", &vcd.timeScale, 0.001f, 0.0f, 0.05f, "%.4f");
			ImGui::SliderFloat("HG Forward eccentricity", &vcd.hgForward, -1.0f, 1.0f);
			ImGui::SliderFloat("Forward silver lining intensity", &vcd.forwardSilverLiningIntensity, 0.0f, 1.0f);
			ImGui::SliderFloat("Silver lining intensity", &vcd.silverLiningIntensity, 0.0f, 1.0f);
			ImGui::SliderFloat("Silver lining sprea", &vcd.silverLiningSpread, 0.0f, 1.0f);
			ImGui::SliderFloat("Density mult", &vcd.densityMult, 0.0f, 8.0f);
			ImGui::SliderFloat("Direct Light mult", &vcd.directLightMult, 0.0f, 4.0f);
			ImGui::ColorEdit3("Ambient bottom color", &vcd.ambientBottomColor.x);
			ImGui::ColorEdit3("Ambient top color", &vcd.ambientTopColor.x);
			ImGui::SliderFloat("Ambient mult", &vcd.ambientMult, 0.0f, 4.0f);
		}

		ImGui::Separator();
		if (ImGui::Button("Choose font"))
		{
			files.clear();
			Engine::utils::FindFilesInDirectory(files, editorManager->GetCurrentProjectDir() + "/*", ".fnt");
			ImGui::OpenPopup("font");
		}

		if (ImGui::BeginPopup("font", 0))
		{
			for (size_t i = 0; i < files.size(); i++)
			{
				if (ImGui::Selectable(files[i].c_str()))
				{
					game->GetRenderingPath()->GetFont().Reload(files[i]);
				}
			}

			ImGui::EndPopup();
		}

		ImGui::Separator();
		ImGui::Checkbox("Enable water", &debugSettings.enableWater);

		if (ImGui::DragFloat("Ocean height", &waterHeight, 0.05f))
			projectedGridWater.SetWaterHeight(waterHeight);

		ImGui::Checkbox("Enable debug draw", &debugSettings.enableDebugDraw);
		ImGui::Checkbox("Enable voxel vis", &debugSettings.enableVoxelVis);

		ImGui::SliderInt("Mip level", &debugSettings.mipLevel, 0, 7);
		int maxSlice = 128 >> debugSettings.mipLevel;
		ImGui::SliderInt("Z Slice", &debugSettings.zSlice, 0, maxSlice);

		ImGui::Checkbox("Enable debug view", &debugSettings.enable);
		if (ImGui::ListBox("Choose debug type:", &selectedDebugType, debugTypesStr))
		{
			if (selectedDebugType == 0)
				debugSettings.type = Engine::DebugType::CSM_SHADOW_MAP;
			else if (selectedDebugType == 1)
				debugSettings.type = Engine::DebugType::BRIGHT_PASS;
			else if (selectedDebugType == 2)
				debugSettings.type = Engine::DebugType::REFLECTION;
			else if (selectedDebugType == 3)
				debugSettings.type = Engine::DebugType::VOXEL_TEXTURE;
			else if (selectedDebugType == 4)
				debugSettings.type = Engine::DebugType::REFRACTION;

		}
	}
	ImGui::EndDock();
}
