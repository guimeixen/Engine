#pragma once

#include "EditorWindow.h"

#include <string>
#include <vector>

class AssetsBrowserWindow : public EditorWindow
{
public:
	AssetsBrowserWindow();

	void Render();

	void SetFiles(const std::string &projectDir);

private:
	std::string currentDir;
	std::vector<std::string> filesInCurrentDir;
	int directoriesDepth = 0;
	char folderName[64];
	bool openContext;
	char fileName[64];
	bool openAddScript;
	bool isFileHovered;
	size_t contextFileIndex;
};

