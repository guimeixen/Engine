#include "imgui.h"
#include "imgui_internal.h"

namespace ImGui
{
	bool ImageButtonID(ImGuiID id, ImTextureID user_texture_id, const ImVec2& size, const ImVec2& uv0, const ImVec2& uv1, int frame_padding, const ImVec4& bg_col, const ImVec4& tint_col)
	{
		ImGuiContext& g = *GImGui;
		ImGuiWindow* window = g.CurrentWindow;
		if (window->SkipItems)
			return false;

		PushID(id);
		const ImGuiID idd = window->GetID("#image");
		PopID();

		const ImVec2 padding = (frame_padding >= 0) ? ImVec2((float)frame_padding, (float)frame_padding) : g.Style.FramePadding;
		return ImageButtonEx(idd, user_texture_id, size, uv0, uv1, padding, bg_col, tint_col);
	}
}