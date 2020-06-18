#include "ConsoleWindow.h"

#include "imgui\imgui.h"

#include "Program\Input.h"

#include <iostream>

ConsoleWindow::ConsoleWindow()
{
	Engine::Log::SetCallbackFunc([this](Engine::LogLevel level, const char *str)
	{
		AddOutput(level, str);
	});
}

void ConsoleWindow::Render()
{
	if (BeginWindow("Console"))
	{
		if (ImGui::Button("Clear"))
		{
			numOutputs = 0;
			outputsIndex = 0;
		}

		ImGui::BeginChild("ConsoleScrollArea", ImVec2(ImGui::GetWindowContentRegionWidth(), ImGui::GetContentRegionAvail().y), true, ImGuiWindowFlags_HorizontalScrollbar);

		for (size_t i = 0; i < numOutputs; i++)
		{
			if (outputs[i].logLevel == Engine::LogLevel::LEVEL_INFO)
			{
				ImGui::Text("Info -");
				ImGui::SameLine();
				ImGui::Text(outputs[i].output);
			}
			else if (outputs[i].logLevel == Engine::LogLevel::LEVEL_WARNING)
			{
				ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.0f, 1.0f), "Warning -");
				ImGui::SameLine();
				ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.0f, 1.0f), outputs[i].output);
			}
			else if (outputs[i].logLevel == Engine::LogLevel::LEVEL_ERROR)
			{
				ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Error -");
				ImGui::SameLine();
				ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), outputs[i].output);
			}
		}
		// If an output was just added then scroll to the bottom
		if (outputAdded)
		{
			ImGui::SetScrollHere();
			outputAdded = false;
		}

		ImGui::EndChild();
	}
	EndWindow();
}

void ConsoleWindow::AddOutput(Engine::LogLevel logLevel, const char *output)
{
	std::cout << outputsIndex << std::endl;
	if (outputsIndex >= MAX_OUTPUTS)
	{
		outputsIndex = MAX_OUTPUTS - 1;
		// Shift all outputs so we put the new output at the end of the array
		for (size_t i = 0; i < numOutputs - 1; i++)
		{
			outputs[i].logLevel = outputs[i + 1].logLevel;
			strcpy(outputs[i].output, outputs[i + 1].output);
		}
	}
	
	strcpy(outputs[outputsIndex].output, output);

	outputs[outputsIndex].logLevel = logLevel;
	
	if (outputsIndex != MAX_OUTPUTS - 1)
		outputsIndex++;
	
	numOutputs++;

	if (numOutputs >= MAX_OUTPUTS)
		numOutputs = MAX_OUTPUTS;

	outputAdded = true;
}
