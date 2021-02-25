#pragma once

#include "EditorWindow.h"
#include "Graphics/Material.h"

class MaterialWindow : public EditorWindow
{
public:
	MaterialWindow();

	void Render();
	void CreateMaterial();
	void Focus() { focus = true; }

	void SetCurrentMaterial(Engine::Material *mat) { currentMaterial = mat; }
	void SetCurrentMaterialInstance(Engine::MaterialInstance *mat) { if (!mat)return; currentMaterialInstance = mat; currentMaterial = mat->baseMaterial; }

private:
	void AddTexture();
	void ChangeShader();
	void ShowTextures();
	void ShowTextureParameters();
	void ShowMaterialParameters();

private:
	Engine::Material *currentMaterial;
	Engine::MaterialInstance *currentMaterialInstance;
	bool focus = false;
	std::vector<std::string> files;
	unsigned int textureIndex = 0;

	unsigned int basePassID;

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

