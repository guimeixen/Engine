#include "imconfig.h"

#include "imgui.h"

bool ImGui::Combo(const char *label, int *currIndex, std::vector<std::string> &values)
{
	if (values.empty())
		return false;

	return Combo(label, currIndex, vector_getter, static_cast<void*>(&values), values.size());
}

bool ImGui::ListBox(const char *label, int *currIndex, std::vector<std::string> &values)
{
	if (values.empty())
		return false;

	return ListBox(label, currIndex, vector_getter, static_cast<void*>(&values), values.size());
}