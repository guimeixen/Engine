#include "AssetsBrowserWindow.h"

#include "Program/Utils.h"
#include "Program/Log.h"
#include "Program/Input.h"
#include "Graphics/Model.h"
#include "EditorManager.h"
#include "AssimpLoader.h"
#include "Graphics/VertexArray.h"

#include <iostream>
#include <filesystem>

#include "imgui/imgui_user.h"

AssetsBrowserWindow::AssetsBrowserWindow()
{
	memset(folderName, 0, 64);
	memset(fileName, 0, 64);
	openContext = false;
	openAddScript = false;
	isFileHovered = false;
	wasDirectoryChanged = false;
	contextFileIndex = 0;
}

void AssetsBrowserWindow::Render()
{
	if (BeginWindow("Assets Browser"))
	{
		if (!isFileHovered && ImGui::IsWindowHovered() && Engine::Input::WasMouseButtonReleased(1))
		{
			ImGui::OpenPopup("Asset context");
			openContext = true;
		}

		if (ImGui::BeginPopup("Asset context"))
		{
			ImGui::Text("Add file");
			if (ImGui::Button("Lua script"))
			{
				openAddScript = true;
				ImGui::CloseCurrentPopup();			
			}
			if (ImGui::Button("Material"))
			{
				ImGui::CloseCurrentPopup();

			}
			ImGui::EndPopup();
		}

		if (openAddScript)
		{
			ImGui::OpenPopup("Create script popup");
			openAddScript = false;
		}

		if (ImGui::BeginPopup("Create script popup"))
		{
			if (ImGui::InputText("Name", fileName, 64, ImGuiInputTextFlags_EnterReturnsTrue))
			{
				// OpenFileWithDefaultProgram needs full path
				std::string path = std::filesystem::current_path().generic_string() + '/' + currentDir + '/' + fileName + ".lua";
				std::ofstream file(path);
				
				file << "require('common')\n\n" << fileName << " = {\n\n\tonInit = function(self, e)\n\t\t\n\tend,\n\n\tonUpdate = function(self, e, dt)\n\t\t\n\tend\n}";
				file.close();

				Engine::utils::OpenFileWithDefaultProgram(path);

				SetFiles(currentDir);

				ImGui::CloseCurrentPopup();
			}
			if (ImGui::Button("Create"))
			{
				//std::string path = currentDir + '/' + fileName + ".lua";
				std::string path = std::filesystem::current_path().generic_string() + '/' + currentDir + '/' + fileName + ".lua";
				std::ofstream file(path);

				file << fileName << " = {\n\n\tonInit = function(self, e)\n\t\t\n\tend,\n\n\tonUpdate = function(self, e, dt)\n\t\t\n\tend\n}";
				file.close();

				Engine::utils::OpenFileWithDefaultProgram(path);

				SetFiles(currentDir);

				ImGui::CloseCurrentPopup();
			}
			ImGui::EndPopup();
		}

		if (ImGui::Button("Create folder"))
			ImGui::OpenPopup("Create folder popup");

		ImGui::SameLine();

		if (ImGui::Button("Create file"))
			ImGui::OpenPopup("Create file popup");

		if (ImGui::BeginPopup("Create folder popup"))
		{
			bool reloadFiles = false;

			if (ImGui::InputText("Name", folderName, 64, ImGuiInputTextFlags_EnterReturnsTrue))
			{
				std::filesystem::create_directory(std::filesystem::current_path().generic_string() + '/' + currentDir + '/' + folderName);
				ImGui::CloseCurrentPopup();
				reloadFiles = true;
			}
			if (ImGui::Button("Create"))
			{
				std::filesystem::create_directory(std::filesystem::current_path().generic_string() + '/' + currentDir + '/' + folderName);
				ImGui::CloseCurrentPopup();
				reloadFiles = true;
			}

			if (reloadFiles)
			{
				SetFiles(currentDir);
			}

			ImGui::EndPopup();
		}

		if (directoriesDepth > 0 && ImGui::Selectable(".", false, ImGuiSelectableFlags_AllowDoubleClick) && ImGui::IsMouseDoubleClicked(0))
		{
			size_t lastSlashIdx = currentDir.find_last_of('/');
			currentDir.erase(lastSlashIdx);
			filesInCurrentDir.clear();
			Engine::utils::FindFilesInDirectory(filesInCurrentDir, currentDir + "/*", "", false);
			directoriesDepth--;
			wasDirectoryChanged = true;

			LoadModelsInCurrentDir();
			editorManager->ReloadThumbnails();
		}

		isFileHovered = false;

		RenderThumbnails();
	}

	if (wasDirectoryChanged)
	{
		// Render the thumbnails
	}


	EndWindow();
}

void AssetsBrowserWindow::Dispose()
{
	for (size_t i = 0; i < modelsForThumbnails.size(); i++)
	{
		delete modelsForThumbnails[i];
	}
}

void AssetsBrowserWindow::RenderThumbnails()
{
	ImGuiStyle& style = ImGui::GetStyle();
	float thumbSize = 80.0f;
	float windowVisibleWidth = ImGui::GetWindowPos().x + ImGui::GetWindowContentRegionMax().x;

	size_t lastIdx = 0;

	Engine::Texture* iconTexture = editorManager->GetIconsTexture();
	Engine::Texture* assetTextureAtlas = editorManager->GetAssetTextureAtlas();

	float increase = 64.0f / 512.0f;

	float uMin = 0.0f;
	float vMin = 0.0f;
	float uMax = increase;
	float vMax = increase;

	for (size_t i = 0; i < filesInCurrentDir.size(); i++)
	{
		size_t lastSlashIdx = filesInCurrentDir[i].find_last_of('/') + 1;		// +1 to removed the slash

		// Right now we assume if the file doesn't have a dot (for the extension) it is a directory
		if (filesInCurrentDir[i].rfind('.') == std::string::npos)
		{
			ImGui::ImageButtonID((ImGuiID)i, iconTexture, ImVec2(thumbSize, thumbSize), ImVec2(0.0f, 0.0f), ImVec2(0.125f, 0.125f));

			if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0))
			{
				currentDir = filesInCurrentDir[i];
				filesInCurrentDir.clear();
				Engine::utils::FindFilesInDirectory(filesInCurrentDir, currentDir + "/*", "", false, false);
				directoriesDepth++;
				wasDirectoryChanged = true;

				LoadModelsInCurrentDir();
				editorManager->ReloadThumbnails();

				break;
			}

		}
		else if (std::strstr(filesInCurrentDir[i].c_str(), ".model") > 0)
		{
			ImGui::ImageButtonID(i, assetTextureAtlas, ImVec2(thumbSize, thumbSize), ImVec2(uMin, vMin), ImVec2(uMax, vMax));

			if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0))
			{

			}

			// Our buttons are both drag sources and drag targets here!
			if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None))
			{
				// Set payload to carry the index of our item (could be anything)
				ImGui::SetDragDropPayload("DND_MODEL_TO_ENTITY", filesInCurrentDir[i].c_str(), filesInCurrentDir[i].length() + 1);

				// Display preview (could be anything, e.g. when dragging an image we could decide to display
				// the filename and a small preview of the image, etc.)
				ImGui::Text(filesInCurrentDir[i].c_str() + lastSlashIdx);			
				ImGui::EndDragDropSource();
			}

			if (uMax < 1.0f)
			{
				uMin += increase;
				uMax += increase;
			}
			else
			{
				uMin = 0.0f;
				uMax = increase;

				vMin += increase;
				vMax += increase;
			}
		}
		else if (std::strstr(filesInCurrentDir[i].c_str(), ".lua") > 0 && std::strstr(filesInCurrentDir[i].c_str(), "_mat.lua") == 0)
		{
			ImGui::ImageButtonID((ImGuiID)i, iconTexture, ImVec2(thumbSize, thumbSize), ImVec2(0.25f, 0.0f), ImVec2(0.375f, 0.125f));

			if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0))
			{
				std::cout << std::filesystem::current_path() << '\n';
				std::string path = std::filesystem::current_path().generic_string() + '/' + filesInCurrentDir[i];

				Engine::utils::OpenFileWithDefaultProgram(path);
			}
		}
		else if (std::strstr(filesInCurrentDir[i].c_str(), ".mat") > 0)
		{
			ImGui::ImageButtonID((ImGuiID)i, iconTexture, ImVec2(thumbSize, thumbSize), ImVec2(0.375f, 0.0f), ImVec2(0.5f, 0.125f));

			if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None))
			{
				ImGui::SetDragDropPayload("DND_MAT_TO_MODEL_MESH", filesInCurrentDir[i].c_str(), filesInCurrentDir[i].length() + 1);
				ImGui::Text(filesInCurrentDir[i].c_str() + lastSlashIdx);
				ImGui::EndDragDropSource();
			}
		}
		else if (std::strstr(filesInCurrentDir[i].c_str(), ".vert") > 0 || std::strstr(filesInCurrentDir[i].c_str(), ".frag") > 0)
		{
			ImGui::ImageButtonID((ImGuiID)i, iconTexture, ImVec2(thumbSize, thumbSize), ImVec2(0.5f, 0.0f), ImVec2(0.625f, 0.125f));
		}
		else
		{
			ImGui::ImageButtonID((ImGuiID)i, iconTexture, ImVec2(thumbSize, thumbSize), ImVec2(0.125f, 0.0f), ImVec2(0.25f, 0.125f));
		}

		if (ImGui::IsItemHovered())
		{
			ImGui::SetTooltip(filesInCurrentDir[i].c_str() + lastSlashIdx);

			if (Engine::Input::WasMouseButtonReleased(1))
			{
				isFileHovered = true;
				contextFileIndex = i;
			}
		}

		float lastButtonX = ImGui::GetItemRectMax().x;
		float nextButtonX = lastButtonX + style.ItemSpacing.x + thumbSize; // Expected position if next button was on same line

		if (i + 1 < filesInCurrentDir.size() && nextButtonX < windowVisibleWidth)
		{
			ImGui::SameLine();
		}
		else
		{
			float buttonSizeX = nextButtonX - lastButtonX;
			float buttonHalfSizeX = buttonSizeX / 2.0f;
			std::string textCut;
			size_t k = 0;
			float prevSideSpace = 0.0f;

			for (size_t j = lastIdx; j <= i; j++)
			{
				size_t lastSlashIdx = filesInCurrentDir[j].find_last_of('/') + 1;		// +1 to removed the slash
				textCut = filesInCurrentDir[j].substr(lastSlashIdx);

				if (textCut.length() > 10)
				{
					textCut.erase(10);
				}

				ImVec2 size = ImGui::CalcTextSize(textCut.c_str());

				float sideSpace = buttonSizeX - size.x;
				sideSpace /= 2.0f;

				if (j > lastIdx)
					ImGui::SameLine(lastButtonX + style.ItemSpacing.x + prevSideSpace + sideSpace);
				else if (j == lastIdx)
					ImGui::SetCursorPosX(sideSpace + style.ItemSpacing.x);			

				ImGui::Text(textCut.c_str());
				
				lastButtonX = ImGui::GetItemRectMax().x;
				nextButtonX = lastButtonX + style.ItemSpacing.x + thumbSize; // Expected position if next button was on same line

				k++;
				prevSideSpace = sideSpace;
			}
			lastIdx = i + 1;
		}
	}

	if (isFileHovered)
		ImGui::OpenPopup("File context popup");

	if (ImGui::BeginPopup("File context popup"))
	{
		std::string path = std::filesystem::current_path().generic_string() + '/' + filesInCurrentDir[contextFileIndex];

		if (ImGui::Button("Open"))
		{
			Engine::utils::OpenFileWithDefaultProgram(path);
			ImGui::CloseCurrentPopup();
		}
		if (std::strstr(path.c_str(), "_mat.lua") > 0)
		{
			if (ImGui::Button("Create material instance from base material"))
			{

			}
		}
		if (ImGui::Button("Delete"))
		{
			int err = std::remove(filesInCurrentDir[contextFileIndex].c_str());
			Engine::Log::Print(Engine::LogLevel::LEVEL_INFO, "%d\n", err);
			ImGui::CloseCurrentPopup();
		}

		ImGui::EndPopup();
	}
}

void AssetsBrowserWindow::SetFiles(const std::string &projectDir)
{
	currentDir = projectDir;
	filesInCurrentDir.clear();
	Engine::utils::FindFilesInDirectory(filesInCurrentDir, projectDir + "/*", "", false, false);

	LoadModelsInCurrentDir();
	editorManager->ReloadThumbnails();
}

void AssetsBrowserWindow::CreateModelMaterial()
{
	Engine::VertexAttribute attribs[4] = {};
	attribs[0].count = 3;						// Position
	attribs[1].count = 2;						// UV
	attribs[2].count = 3;						// Normal
	attribs[3].count = 3;						// Tangent

	attribs[0].offset = 0;
	attribs[1].offset = 3 * sizeof(float);
	attribs[2].offset = 5 * sizeof(float);
	attribs[3].offset = 8 * sizeof(float);

	Engine::VertexInputDesc desc = {};
	desc.stride = sizeof(Engine::VertexPOS3D_UV_NORMAL_TANGENT);
	desc.attribs = { attribs[0], attribs[1], attribs[2], attribs[3] };

	modelThumbnailMat = game->GetRenderer()->CreateMaterialInstanceFromBaseMat(game->GetScriptManager(), "Data/Materials/model_thumbnail_mat.lua", { desc });
}

void AssetsBrowserWindow::LoadModelsInCurrentDir()
{
	modelsInCurDir.clear();

	// Load the models in the current directory if they haven't been already loaded
	for (size_t i = 0; i < filesInCurrentDir.size(); i++)
	{
		if (std::strstr(filesInCurrentDir[i].c_str(), ".fbx") > 0 || std::strstr(filesInCurrentDir[i].c_str(), ".obj") > 0 || std::strstr(filesInCurrentDir[i].c_str(), ".FBX") > 0)
		{
			// Check if we already have the model loaded		
			std::string newPath = Engine::utils::RemoveExtensionFromFilePath(filesInCurrentDir[i]) + "model";
			bool loadedSuccessfuly = false;
			bool found = false;

			for (size_t j = 0; j < modelsForThumbnails.size(); j++)
			{
				if (modelsForThumbnails[j]->GetPath() == newPath)
				{
					found = true;
					modelsInCurDir.push_back(modelsForThumbnails[j]);
					break;
				}
			}

			if (found)
				continue;
			
			// Check if the model in the custom format exists, if not loaded the obj, fbx, etc with Assimp and save it in the custom format
			if (Engine::utils::DirectoryExists(newPath) == false)
			{
				loadedSuccessfuly = Engine::AssimpLoader::LoadModel(game, filesInCurrentDir[i], {});
			}
			else
			{
				loadedSuccessfuly = true;		// .model file already exists
			}

			if (loadedSuccessfuly)
			{
				Engine::Model* model = new Engine::Model(game->GetRenderer(), game->GetScriptManager(), newPath, {});

				Engine::Log::Print(Engine::LogLevel::LEVEL_INFO, "Loading model for thumbnail: %s\n", newPath.c_str());

				for (size_t i = 0; i < model->GetMeshesAndMaterials().size(); i++)
				{
					model->SetMeshMaterial(i, modelThumbnailMat);
				}

				modelsForThumbnails.push_back(model);
				modelsInCurDir.push_back(model);
			}		
		}
	}

	// Refind the files in the current dir so the new .model files show up
	filesInCurrentDir.clear();
	Engine::utils::FindFilesInDirectory(filesInCurrentDir, currentDir + "/*", "", false, false);
}
