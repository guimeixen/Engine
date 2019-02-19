#include "AssetsBrowserWindow.h"

#include "Program/Utils.h"
#include "Program/Log.h"
#include "EditorManager.h"

AssetsBrowserWindow::AssetsBrowserWindow()
{
}

void AssetsBrowserWindow::Render()
{
	if (BeginWindow("Assets Browser"))
	{
		for (size_t i = 0; i < filesInCurrentDir.size(); i++)
		{
			ImGui::Selectable(filesInCurrentDir[i].c_str());
		}
	}
	EndWindow();
}

void AssetsBrowserWindow::SetFiles(const std::string &projectDir)
{
	Engine::utils::FindFilesInDirectory(filesInCurrentDir, editorManager->GetCurrentProjectDir() + "/*", "");
}
