#include "AssetsBrowserWindow.h"

#include "Program/Utils.h"
#include "Program/Log.h"
#include "EditorManager.h"

#include <iostream>
#include <filesystem>
#include <Windows.h>
#include <shellapi.h>


AssetsBrowserWindow::AssetsBrowserWindow()
{
	memset(folderName, 0, 64);
}

void AssetsBrowserWindow::Render()
{
	if (BeginWindow("Assets Browser"))
	{
		if (ImGui::Button("Create folder"))
			ImGui::OpenPopup("Create folder popup");

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
						ShellExecuteA(0, 0, path.c_str(), 0, 0, SW_SHOW);
					}
				}
			}

			/*if (ImGui::IsItemHovered())
			{
				if (std::strstr(filesInCurrentDir[i].c_str(), ".fbx") > 0)
				{
					// Load model
					// Render to fbo
					// Render fbo texture has a thumbnail

				}
			}*/		
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
