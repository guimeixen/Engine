#include "TerrainWindow.h"

#include "Game/Game.h"
#include "Graphics/Material.h"
#include "Graphics/Texture.h"
#include "Graphics/Renderer.h"
#include "Graphics/Model.h"
#include "Graphics/VertexArray.h"
#include "EditorManager.h"
#include "Program/Utils.h"
#include "Commands.h"

#include "imgui/imgui.h"
#include "imgui/imgui_dock.h"

#include <iostream>

#include <Windows.h>

TerrainWindow::TerrainWindow()
{
	currentTerrain = nullptr;
	terrainEditMode = 0;
	flattenHeight = 0.0f;
}

void TerrainWindow::Init(Engine::Game *game, EditorManager *editorManager)
{
	EditorWindow::Init(game, editorManager);
	this->currentTerrain = game->GetTerrain();

	texturesPopupNames[0] = "Choose red texture:";
	texturesPopupNames[1] = "Choose green texture:";
	texturesPopupNames[2] = "Choose blue texture:";
	texturesPopupNames[3] = "Choose black texture:";
	texturesPopupNames[4] = "Choose red normal texture:";
	texturesPopupNames[5] = "Choose splatmap texture:";
}

void TerrainWindow::Render()
{
	isSelected = false;

	if (BeginWindow("Terrain Editor"))
	{
		isSelected = true;

		currentTerrain = game->GetTerrain();

		if (!currentTerrain)
		{
			EndWindow();
			return;
		}

		heightScale = currentTerrain->GetHeightScale();
		brushRadius = currentTerrain->GetBrushRadius();
		brushStrength = currentTerrain->GetBrushStrength();
		vegBrushRadius = currentTerrain->GetVegetationBrushRadius();
		flattenHeight = currentTerrain->GetFlattenHeight();

		if (ImGui::CollapsingHeader("Heightmap"))
		{
			resStr = "Current terrain resolution: " + std::to_string(currentTerrain->GetResolution());
			ImGui::Text(resStr.c_str());

			if (ImGui::Button("Change heightmap"))
			{
				std::string dir = editorManager->GetCurrentProjectDir() + "/*";

				files.clear();
				FindFilesInDirectory(dir, ".png");

				ImGui::OpenPopup("Choose heightmap:");
			}

			if (ImGui::BeginDragDropTarget())
			{
				const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("DND_HEIGHTMAP_TO_TERRAIN");

				if (payload)
				{
					const char* heightmapPath = (const char*)payload->Data;
					Engine::Log::Print(Engine::LogLevel::LEVEL_INFO, "%s\n", heightmapPath);

					currentTerrain->SetHeightmap(heightmapPath);
				}
				ImGui::EndDragDropTarget();
			}

			if (ImGui::BeginPopup("Choose heightmap:"))
			{
				for (size_t i = 0; i < files.size(); i++)
				{
					if (ImGui::Selectable(files[i].c_str()))
					{
						currentTerrain->SetHeightmap(files[i]);
					}
				}

				ImGui::EndPopup();
			}
		}

		if (ImGui::CollapsingHeader("Terrain Editing"))
		{
			if (currentTerrain->IsEditable() == false)
			{
				if (ImGui::Button("Enable terrain editing"))
					currentTerrain->EnableEditing();
			}
			else
			{
				if (ImGui::Button("Disable terrain editing"))
					currentTerrain->DisableEditing();

				ImGui::Text("Terrain edit mode:");

				if (ImGui::RadioButton("Raise", &terrainEditMode, 0))
					currentTerrain->SetEditMode(Engine::TerrainEditMode::RAISE);

				ImGui::SameLine();

				if (ImGui::RadioButton("Lower", &terrainEditMode, 1))
					currentTerrain->SetEditMode(Engine::TerrainEditMode::LOWER);

				ImGui::SameLine();

				if (ImGui::RadioButton("Flatten", &terrainEditMode, 2))
					currentTerrain->SetEditMode(Engine::TerrainEditMode::FLATTEN);

				ImGui::SameLine();

				if (ImGui::RadioButton("Smooth", &terrainEditMode, 3))
					currentTerrain->SetEditMode(Engine::TerrainEditMode::SMOOTH);

				if (ImGui::SliderFloat("Brush radius", &brushRadius, 0.1f, 50.0f))
				{
					currentTerrain->SetBrushRadius(brushRadius);
				}
				if (ImGui::SliderFloat("Brush strength", &brushStrength, 0.1f, 10.0f))
				{
					currentTerrain->SetBrushStrength(brushStrength);
				}

				if (terrainEditMode == 2)
				{
					if (ImGui::SliderFloat("Flatten Height", &flattenHeight, 0.0f, 255.0f))
						currentTerrain->SetFlattenHeight(flattenHeight);
				}
			}
		}

		if (ImGui::CollapsingHeader("Material"))
		{
			if (ImGui::Button("Change material"))
			{
				std::string dir = editorManager->GetCurrentProjectDir() + "/*";

				files.clear();
				FindFilesInDirectory(dir, ".mat");

				ImGui::OpenPopup("Choose material:");
			}

			if (ImGui::BeginPopup("Choose material:"))
			{
				for (size_t i = 0; i < files.size(); i++)
				{
					if (ImGui::Selectable(files[i].c_str()))
						currentTerrain->SetMaterial(files[i]);
				}

				ImGui::EndPopup();
			}

			Engine::MaterialInstance* matInstance = currentTerrain->GetMaterialInstance();
			const std::vector<Engine::Texture*>& textures = matInstance->textures;

			int popupNamesID = 0;

			// Start at 1 to skip the heightmap
			for (size_t i = 1; i < textures.size(); i++)
			{
				//ImGui::Text(textures[i]->GetPath().c_str());
				ImGui::Text(texturesPopupNames[i - 1].c_str());
				ImGui::SameLine();

				if (ImGui::Button("Load..."))
				{
					std::string dir = editorManager->GetCurrentProjectDir() + "/*";

					files.clear();
					FindFilesInDirectory(dir, ".dds");
					FindFilesInDirectory(dir, ".png");
					FindFilesInDirectory(dir, ".ktx");

					ImGui::OpenPopup(texturesPopupNames[popupNamesID].c_str());
				}
				if (ImGui::BeginPopup(texturesPopupNames[popupNamesID].c_str()))
				{
					for (size_t j = 0; j < files.size(); j++)
					{
						if (ImGui::Selectable(files[j].c_str()))
						{
							game->GetRenderer()->WaitIdle();

							Engine::Texture* tex = textures[i];
							const Engine::TextureParams& params = tex->GetTextureParams();
							tex->RemoveReference();
							game->GetRenderer()->RemoveTexture(tex);

							tex = game->GetRenderer()->CreateTexture2D(files[j], params);
							game->GetRenderer()->UpdateMaterialInstance(matInstance);
						}
					}

					ImGui::EndPopup();
					popupNamesID++;
				}
			}
		}

		/*if (ImGui::DragFloat("Height scale (NOT WORKING)", &heightScale, 0.01f))
		{
			currentTerrain->SetHeightScale(heightScale);
		}*/

		/*if (ImGui::Button("Save material"))
		{
			std::ofstream file(editorManager->GetCurrentProjectDir() + "/terrain_" + game->GetScenes()[game->GetCurrentSceneId()].name + ".dat");

			if (!file.is_open())
			{
				std::cout << "Error! Failed to save terrain!\n";
				return;
			}

			file << "mat=" << matInstance->path << '\n';

			if (vegetation.size() > 0)
			{
				file << "veg=" << folder << "vegetation_" << sceneName << ".dat\n";		// Replace with path to vegetation file
			}
		}*/

		if (ImGui::CollapsingHeader("Vegetation"))
		{
			popupOpen = false;

			if (ImGui::DragFloat("Vegetation brush radius", &vegBrushRadius, 0.1f, 0.0f))
			{
				currentTerrain->SetVegetationBrushRadius(vegBrushRadius);
			}

			if (ImGui::Button("Reseat vegetation"))
			{
				currentTerrain->ReseatVegetation();
			}

			if (ImGui::CollapsingHeader("Vegetation list"))
			{
				if (ImGui::Button("Add new"))
				{
					ImGui::OpenPopup("Choose model");
					std::string dir = editorManager->GetCurrentProjectDir() + "/*";
					files.clear();
					FindFilesInDirectory(dir, ".obj");
					FindFilesInDirectory(dir, ".fbx");
					FindFilesInDirectory(dir, ".FBX");
				}
				if (ImGui::BeginPopup("Choose model"))
				{
					popupOpen = true;
					if (files.size() > 0)
						AddNewObject();
					else
						ImGui::Text("No models found on project folder.");

					ImGui::EndPopup();
				}

				std::vector<Engine::Vegetation>& veg = currentTerrain->GetVegetation();
				if (ImGui::TreeNode("Selection"))
				{
					if (veg.size() > selection.size())
						selection.resize(veg.size());

					for (size_t i = 0; i < veg.size(); i++)
					{
						if (ImGui::Selectable(veg[i].model->GetPath().c_str(), selection[i]))
						{
							selection[i] = !selection[i];

							if (selection[i])
							{
								selected = true;
								id = i;
							}
							else
							{
								selected = false;
								id = -1;
							}
						}
					}
					ImGui::TreePop();
				}

				if (selected)
				{
					Engine::Vegetation& v = veg[id];
					HandleVegModel(v);

					ImGui::Text(std::to_string(v.count).c_str());

					if (ImGui::DragInt("Density", &v.density, 0.1f, 1))
					{
						if (v.density < 1)
							v.density = 1;
					}
					if (ImGui::DragFloat("Lod 1 start dist", &v.lod1Dist, 1.0f, 0.0f))
					{
						if (v.lod1Dist < 0.0f)
							v.lod1Dist = 0.0f;
					}
					if (ImGui::DragFloat("Lod 2 start dist", &v.lod2Dist, 1.0f, 0.0f))
					{
						if (v.lod2Dist < 0.0f)
							v.lod2Dist = 0.0f;
					}
					if (ImGui::DragFloat("Max Slope", &v.maxSlope, 0.05f, 0.0f, 1.0f))
					{
						if (v.maxSlope < 0.0f)
							v.maxSlope = 0.0f;
						else if (v.maxSlope > 1.0f)
							v.maxSlope = 1.0f;
					}
					if (ImGui::DragFloat("Min Scale", &v.minScale, 0.05f, 0.0f))
					{
						if (v.minScale < 0.0f)
							v.minScale = 0.0f;
					}
					if (ImGui::DragFloat("Max Scale", &v.maxScale, 0.05f, 0.0f))
					{
						if (v.maxScale < 0.0f)
							v.maxScale = 0.0f;
					}
					ImGui::DragFloat("Height Offset", &v.heightOffset, 0.05f);
					ImGui::Checkbox("Generate colliders", &v.generateColliders);
					ImGui::Checkbox("Generate obstacles", &v.generateObstacles);

					if (!choosingLOD2)
						AddModelLOD1(v);

					if (!choosingLOD1)
						AddModelLOD2(v);
				}
			}
		}		
	}
	EndWindow();
}

void TerrainWindow::Paint(const glm::vec2 &mousePos)
{
	if (!currentTerrain || popupOpen)
		return;

	ids.clear();
	for (size_t i = 0; i < selection.size(); i++)
	{
		if (selection[i])
			ids.push_back(i);
	}

	glm::vec3 dir = Engine::utils::GetRayDirection(mousePos, game->GetMainCamera());

	//currentTerrain->PaintVegetation(ids, game->GetMainCamera()->GetPosition(), dir);

	VegetationPlacementCommand *cmd = new VegetationPlacementCommand(currentTerrain, ids, game->GetMainCamera()->GetPosition(), dir);
	editorManager->GetUndoStack().push(cmd);
	//cmd->Execute();
}

void TerrainWindow::FindFilesInDirectory(const std::string &dir, const char *extension)
{
	WIN32_FIND_DATAA findData;
	HANDLE h = FindFirstFileA(dir.c_str(), &findData);

	if (INVALID_HANDLE_VALUE == h)
	{
		std::cout << "Ended search for files\n";
		return;
	}

	do
	{
		if (findData.cFileName[0] != '.' && findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		{
			std::string path = dir;
			path.pop_back();		// Remove the * necessary to find the files
			path += findData.cFileName;
			path += "/*";
			FindFilesInDirectory(path.c_str(), extension);
		}
		else if (findData.cFileName[0] != '.' && (std::strstr(findData.cFileName, extension) > 0))
		{
			std::string path = dir;
			path.pop_back();		// Remove the * necessary to find the files
			files.push_back(path + std::string(findData.cFileName));
		}

	} while (FindNextFileA(h, &findData));

	FindClose(h);
}

void TerrainWindow::AddNewObject()
{
	for (size_t i = 0; i < files.size(); i++)
	{
		if (ImGui::Selectable(files[i].c_str()))
		{
			currentTerrain->AddVegetation(files[i]);
			size_t vegSize = currentTerrain->GetVegetation().size();

			selection.resize(vegSize);
		}
	}
}

void TerrainWindow::AddModelLOD1(Engine::Vegetation &v)
{
	if (ImGui::Button("Add model LOD1"))
	{
		ImGui::OpenPopup("Choose model LOD1");
		std::string dir = editorManager->GetCurrentProjectDir() + "/*";
		files.clear();
		FindFilesInDirectory(dir, ".obj");
		FindFilesInDirectory(dir, ".fbx");
		
	}

	choosingLOD1 = false;

	if (ImGui::BeginPopup("Choose model LOD1"))
	{
		popupOpen = true;
		choosingLOD1 = true;
		if (files.size() > 0)
		{		
			for (size_t i = 0; i < files.size(); i++)
			{
				if (ImGui::Selectable(files[i].c_str()))
				{
					currentTerrain->ChangeVegetationModel(v, 1, files[i]);
				}
			}
		}
		else
			ImGui::Text("No models found on project folder.");

		ImGui::EndPopup();
	}
}

void TerrainWindow::AddModelLOD2(Engine::Vegetation &v)
{
	if (ImGui::Button("Add model LOD2"))
	{
		ImGui::OpenPopup("Choose model LOD2");
		std::string dir = editorManager->GetCurrentProjectDir() + "/*";
		files.clear();
		FindFilesInDirectory(dir, ".obj");
		FindFilesInDirectory(dir, ".fbx");
	}

	choosingLOD2 = false;

	if (ImGui::BeginPopup("Choose model LOD2"))
	{
		popupOpen = true;
		choosingLOD2 = true;
		if (files.size() > 0)
		{
			for (size_t i = 0; i < files.size(); i++)
			{
				if (ImGui::Selectable(files[i].c_str()))
				{
					currentTerrain->ChangeVegetationModel(v, 2, files[i]);
				}
			}
		}
		else
			ImGui::Text("No models found on project folder.");

		ImGui::EndPopup();
	}
}

void TerrainWindow::HandleVegModel(const Engine::Vegetation &v)
{
	if (ImGui::CollapsingHeader(v.model->GetPath().c_str()))
	{
		ImGui::Indent();
		const std::vector<Engine::MeshMaterial> &meshesAndMaterials = v.model->GetMeshesAndMaterials();

		for (size_t i = 0; i < meshesAndMaterials.size(); i++)
		{
			if (ImGui::TreeNodeEx(std::string("Mesh " + std::to_string(i)).c_str(), ImGuiTreeNodeFlags_DefaultOpen))
			{
				const Engine::MeshMaterial &mm = meshesAndMaterials[i];

				std::string matPath = "Material: " + mm.mat->path;

				if (ImGui::Selectable(matPath.c_str()))
				{
					ImGui::OpenPopup("Choose material");
					std::string dir = editorManager->GetCurrentProjectDir() + "/*";
					files.clear();
					FindFilesInDirectory(dir, ".mat");
				}

				if (ImGui::BeginPopup("Choose material"))
				{
					popupOpen = true;
					if (files.size() > 0)
					{
						for (size_t j = 0; j < files.size(); j++)
						{
							if (ImGui::Selectable(files[j].c_str()))
							{
								v.model->SetMeshMaterial((unsigned short)i, game->GetRenderer()->CreateMaterialInstance(game->GetScriptManager(), files[j], mm.mesh.vao->GetVertexInputDescs()));
								if(v.modelLOD1)
									v.modelLOD1->SetMeshMaterial((unsigned short)i, v.model->GetMaterialInstanceOfMesh((unsigned short)i));
								if(v.modelLOD2)
									v.modelLOD2->SetMeshMaterial((unsigned short)i, v.model->GetMaterialInstanceOfMesh((unsigned short)i));
							}
						}
					}
					else
						ImGui::Text("No materials found on project folder.");

					ImGui::EndPopup();
				}

				ImGui::TreePop();
			}
		}
		ImGui::Unindent();
	}
}
