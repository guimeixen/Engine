#pragma once

#include <vector>

#include "Game\Game.h"
#include "Graphics\Material.h"

class EditorManager;

class MaterialWindow
{
public:
	MaterialWindow();
	~MaterialWindow();

	void Init(Engine::Game *game, EditorManager *editorManager);
	void Render();

	void Show(bool show) { showWindow = show; }
	bool IsVisible() const { return showWindow; }
	void Focus() { focus = true; }

	void SetCurrentMaterial(Engine::Material *mat) { currentMaterial = mat; }
	void SetCurrentMaterialInstance(Engine::MaterialInstance *mat) { if (!mat)return; currentMaterialInstance = mat; currentMaterial = mat->baseMaterial; }

private:
	void AddTexture();
	void ShowTextureParameters();
	void ShowMaterialParameters();

private:
	Engine::Game *game;
	EditorManager *editorManager;
	Engine::Material *currentMaterial;
	Engine::MaterialInstance *currentMaterialInstance;
	bool showWindow = true;
	bool focus = false;
	std::vector<std::string> files;
	unsigned int textureIndex = 0;

	int textureFilterComboIndex = 0;
	int textureWrapComboIndex = 0;
	int matParamTypeComboIndex = 0;

	float tempF = 0.0f;
	int tempI = 0;
	glm::vec2 tempv2;
	glm::vec4 tempv4;
	glm::vec3 tempColV3;
	glm::vec4 tempColV4;
};

