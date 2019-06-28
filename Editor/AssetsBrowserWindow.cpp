#include "AssetsBrowserWindow.h"

#include "Program/Utils.h"
#include "Program/Log.h"
#include "Program/Input.h"
#include "EditorManager.h"

#include <iostream>
#include <filesystem>

AssetsBrowserWindow::AssetsBrowserWindow()
{
	memset(folderName, 0, 64);
	memset(fileName, 0, 64);
	openContext = false;
	openAddScript = false;
	isFileHovered = false;
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
				std::string path = std::experimental::filesystem::current_path().generic_string() + '/' + currentDir + '/' + fileName + ".lua";
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
				std::string path = std::experimental::filesystem::current_path().generic_string() + '/' + currentDir + '/' + fileName + ".lua";
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
				std::experimental::filesystem::create_directory(std::experimental::filesystem::current_path().generic_string() + '/' + currentDir + '/' + folderName);
				ImGui::CloseCurrentPopup();
				reloadFiles = true;
			}
			if (ImGui::Button("Create"))
			{
				std::experimental::filesystem::create_directory(std::experimental::filesystem::current_path().generic_string() + '/' + currentDir + '/' + folderName);
				ImGui::CloseCurrentPopup();
				reloadFiles = true;
			}

			if (reloadFiles)
			{
				SetFiles(editorManager->GetCurrentProjectDir());
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
		}


		isFileHovered = false;

		for (size_t i = 0; i < filesInCurrentDir.size(); i++)
		{
			size_t lastSlashIdx = filesInCurrentDir[i].find_last_of('/') + 1;		// +1 to removed the slash
			if (ImGui::Selectable(filesInCurrentDir[i].c_str() + lastSlashIdx, false, ImGuiSelectableFlags_AllowDoubleClick) && ImGui::IsMouseDoubleClicked(0))
			{
				// Right now we assume if the file doesn't have a dot (for the extension) it is a directory
				if (filesInCurrentDir[i].rfind('.') == std::string::npos)
				{
					currentDir = filesInCurrentDir[i];
					filesInCurrentDir.clear();
					Engine::utils::FindFilesInDirectory(filesInCurrentDir, currentDir + "/*", "", false, false);
					directoriesDepth++;
				}
				else
				{
					// Try open the file
					if (std::strstr(filesInCurrentDir[i].c_str(), ".lua") > 0)
					{
						std::cout << std::experimental::filesystem::current_path() << '\n';
						std::string path = std::experimental::filesystem::current_path().generic_string() + '/' + filesInCurrentDir[i];

						Engine::utils::OpenFileWithDefaultProgram(path);
					}
				}
			}

			if (ImGui::IsItemHovered() && Engine::Input::WasMouseButtonReleased(1))
			{
				isFileHovered = true;
				contextFileIndex = i;

				/*if (std::strstr(filesInCurrentDir[i].c_str(), ".fbx") > 0)
				{
					// Load model
					// Render to fbo
					// Render fbo texture has a thumbnail

				}*/
			}
		}

		if (isFileHovered)
			ImGui::OpenPopup("File context popup");

		if (ImGui::BeginPopup("File context popup"))
		{
			if (ImGui::Button("Open"))
			{
				std::string path = std::experimental::filesystem::current_path().generic_string() + '/' + filesInCurrentDir[contextFileIndex];
				Engine::utils::OpenFileWithDefaultProgram(path);
				ImGui::CloseCurrentPopup();
			}
			if (ImGui::Button("Delete"))
			{
				std::remove(filesInCurrentDir[contextFileIndex].c_str());
				ImGui::CloseCurrentPopup();
			}

			ImGui::EndPopup();
		}

		//ImGui::GetWindowDrawList()->AddRectFilled(ImGui::GetCursorScreenPos(), ImVec2(50.0f, 50.0f), IM_COL32(255, 0, 0, 255));
	}
	EndWindow();
}

void AssetsBrowserWindow::SetFiles(const std::string &projectDir)
{
	currentDir = projectDir;
	filesInCurrentDir.clear();
	Engine::utils::FindFilesInDirectory(filesInCurrentDir, projectDir + "/*", "", false, false);
}
