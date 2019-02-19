#pragma once

#include "EditorWindow.h"

class AssetsBrowserWindow : public EditorWindow
{
public:
	AssetsBrowserWindow();

	void Render();

	void SetFiles(const std::string &projectDir);

private:
	std::string currentDir;
	std::vector<std::string> filesInCurrentDir;
};

