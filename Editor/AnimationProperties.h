#pragma once

#include "EditorWindow.h"
#include "Graphics\Animation\AnimatedModel.h"

#include <vector>

class AnimationProperties : public EditorWindow
{
public:
	AnimationProperties();
	~AnimationProperties();

	void Render();

private:
	void HandleAddParameter(std::vector<Engine::ParameterDesc> &parameters);
	void HandleLinkOptions(const std::vector<Engine::ParameterDesc> &parameters);

private:
	Engine::Entity curEntity;
	Engine::AnimationController *curAnimController;

	std::vector<char*> paramNames;
	std::vector<int> intCombos;
	std::vector<int> floatCombos;
	std::vector<int> boolCombos;

	std::vector<int> inputInts;
	std::vector<float> inputFloats;

	float transitionTime = 0.0f;
};

