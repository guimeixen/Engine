#pragma once

#include "EditorWindow.h"

#include <string>
#include <vector>

namespace Engine
{
	class Model;
	struct MaterialInstance;
}

class AssetsBrowserWindow : public EditorWindow
{
public:
	AssetsBrowserWindow();

	void Render();
	void Dispose();

	void RenderThumbnails();

	void SetFiles(const std::string &projectDir);
	void CreateModelMaterial();

	const std::vector<Engine::Model*>& GetModelsInCurrentDir() const { return modelsInCurDir; }
	Engine::MaterialInstance* GetModelThumbnailMaterial() const { return modelThumbnailMat; }

private:
	void LoadModelsInCurrentDir();

private:
	std::string currentDir;
	std::vector<std::string> filesInCurrentDir;
	int directoriesDepth = 0;
	char folderName[64];
	bool openContext;
	char fileName[64];
	bool openAddScript;
	bool isFileHovered;
	bool wasDirectoryChanged;
	bool dragAndDropModelPayloadSet;
	size_t contextFileIndex;

	std::vector<Engine::Model*> modelsForThumbnails;
	std::vector<Engine::Model*> modelsInCurDir;
	Engine::MaterialInstance* modelThumbnailMat;
};

