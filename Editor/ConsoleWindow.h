#pragma once

#include "EditorWindow.h"

#include "Program\Log.h"

struct ConsoleOutput
{
	char output[2048];
	Engine::LogLevel logLevel;
};

class ConsoleWindow : public EditorWindow
{
public:
	ConsoleWindow();

	void Render();

	void AddOutput(Engine::LogLevel logLevel, const char *output);

private:
	static const unsigned int MAX_OUTPUTS = 64;
	ConsoleOutput outputs[MAX_OUTPUTS];
	unsigned int outputsIndex = 0;
	unsigned int numOutputs = 0;
	bool outputAdded = false;
};

