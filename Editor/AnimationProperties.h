#pragma once

#include "Graphics\Animation\AnimatedModel.h"

#include <vector>

class Engine::Game;
class EditorManager;

class AnimationProperties
{
public:
	AnimationProperties();
	~AnimationProperties();

	void Init(Engine::Game *game, EditorManager *editorManager);
	void Render();

	void Show(bool show) { showWindow = show; }
	bool IsVisible() const { return showWindow; }

private:
	void HandleAddParameter(std::vector<Engine::ParameterDesc> &parameters);
	void HandleLinkOptions(const std::vector<Engine::ParameterDesc> &parameters);

private:
	Engine::Game *game;
	EditorManager *editorManager;
	bool showWindow = true;

	Engine::Object *curObj;
	Engine::AnimationController *curAnimController;

	std::vector<char*> paramNames;
	std::vector<int> intCombos;
	std::vector<int> floatCombos;
	std::vector<int> boolCombos;

	std::vector<int> inputInts;
	std::vector<float> inputFloats;

	float transitionTime = 0.0f;
};

