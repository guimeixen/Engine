#include "SkeletonTreeWindow.h"

#include "EditorManager.h"
#include "Program\Input.h"

#include "imgui\imgui.h"
#include "imgui\imgui_dock.h"

#include <iostream>

void SkeletonTreeWindow::Render()
{
	if (BeginWindow("Skeleton Tree"))
	{
		Engine::Entity e = editorManager->GetObjectWindow().GetSelectedEntity();

		if (!e.IsValid())
		{
			ImGui::Text("There's no entity selected.");
			EndWindow();
			return;
		}

		if (game->GetModelManager().HasModel(e) == false)
		{
			ImGui::Text("The currently selected entity doesn't have an animated model.");
			EndWindow();
			return;
		}

		Engine::Model *m = game->GetModelManager().GetModel(e);

		if (m->GetType() != Engine::ModelType::ANIMATED)
		{
			ImGui::Text("The currently selected entity doesn't have an animated model.");
			EndWindow();
			return;
		}

		Engine::AnimatedModel *am = static_cast<Engine::AnimatedModel*>(m);

		hovered = false;
		RenderSkeleton(am->GetRootBone());

		if (hovered)
		{
			ImGui::OpenPopup("context");
			hovered = false;
		}

		if (ImGui::BeginPopup("context"))
		{
			const std::vector<EditorName> &editorNames = editorManager->GetEditorNameManager().GetNames();
			std::vector<std::string> names(editorNames.size());
			for (size_t i = 0; i < names.size(); i++)
			{
				names[i] = editorNames[i].name;
			}

			ImGui::Text("Create connection with object");

			if (ImGui::ListBox("Choose object:", &selectedEntityIndex, names))
			{
				am->AddBoneAttachment(game, selectedBone, editorNames[selectedEntityIndex].e);
				ImGui::CloseCurrentPopup();
			}
			
			ImGui::EndPopup();
		}

		/*ImGui::Separator();
		ImGui::Text("Connections: ");
		const Engine::BoneAttachment *boneConnections = am->GetBoneAttachments();

		const Engine::BoneAttachment *curConnection = boneConnections;
		for (unsigned short i = 0; i < am->GetBoneAttachmentsCount(); i++)
		{
			ImGui::Text(curConnection->bone->name.c_str());
			ImGui::SameLine();
			if (ImGui::Button("Go to object"))
			{
				editorManager->GetGizmo().SetSelectedObject(curConnection->obj);
				editorManager->GetObjectWindow().SetObject(curConnection->obj);
			}
			curConnection++;
		}*/
	}
	EndWindow();
}

void SkeletonTreeWindow::RenderSkeleton(Engine::Bone *bone)
{
	ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_OpenOnDoubleClick;
	if (bone->children.size() == 0)
		flags |= ImGuiTreeNodeFlags_Leaf;
	
	if (ImGui::TreeNodeEx(bone->name.c_str(), flags))
	{
		if (!hovered && Engine::Input::IsMouseButtonDown(1))
		{
			hovered = ImGui::IsItemClicked(1);
			if (hovered)
			{
				selectedBone = bone;
				std::cout << bone->name << '\n';
			}
		}

		for (size_t i = 0; i < bone->children.size(); i++)
			RenderSkeleton(bone->children[i]);

		ImGui::TreePop();
	}
}
