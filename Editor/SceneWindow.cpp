#include "SceneWindow.h"

#include "Game\Game.h"

#include "Program\Input.h"
#include "Program\StringID.h"

#include "EditorManager.h"

#include "Gizmo.h"
#include "Game/UI/UIManager.h"
#include "Game/Game.h"

#include "imgui\imgui_dock.h"

#include <iostream>
#include <fstream>

SceneWindow::SceneWindow()
{
	gizmo = nullptr;
	std::memset(objectName, 0, 256);
}

SceneWindow::~SceneWindow()
{
}

void SceneWindow::Init(Engine::Game *game, EditorManager *editorManager, Gizmo *gizmo)
{
	this->game = game;
	this->editorManager = editorManager;
	this->gizmo = gizmo;
	transformManager = &game->GetTransformManager();
}

void SceneWindow::Render()
{
	if (ImGui::BeginDock("Scene", &showWindow))
	{
		if (!editorManager->IsProjectOpen())
		{
			ImGui::EndDock();
			return;
		}

		const std::vector<EditorName> &names = editorManager->GetEditorNameManager().GetNames();

		imguiID = 0;
		for (unsigned int i = 0; i < editorManager->GetEditorNameManager().GetNameCount(); i++)
		{
			const EditorName &en = names[i];
			if (transformManager->HasParent(en.e) == false)			// Only render the node if the entity is not a child of another. Children get rendered when rendering the parent
				RenderEntityName(en);
		}

		// Context menu
		if (openPopup)
		{
			ImGui::OpenPopup(popupID);
			openPopup = false;
		}

		if (ImGui::BeginPopup(popupID))
		{
			popupOpen = true;
			std::strcpy(objectName, editorManager->GetEditorNameManager().GetName(selectedEntity));		// Needed so the name of the previous object doesn't show up when editing another object
			if (ImGui::InputText("Rename", objectName, 128, ImGuiInputTextFlags_EnterReturnsTrue))
			{
				editorManager->GetEditorNameManager().SetName(selectedEntity, objectName);			
				ImGui::CloseCurrentPopup();
			}

			// Only appears unparent if has parent
			if (game->GetTransformManager().HasParent(selectedEntity) && ImGui::Button("Unparent"))
			{
				game->GetTransformManager().RemoveParent(selectedEntity);
				ImGui::CloseCurrentPopup();
			}

			if (ImGui::Button("Delete"))
			{
				game->GetEntityManager().Destroy(selectedEntity);
				editorManager->GetObjectWindow().DeselectEntity();
				editorManager->GetGizmo().DeselectEntity();
				ImGui::CloseCurrentPopup();
			}

			/*if (ImGui::Button("Duplicate"))
			{
				Engine::Object *obj = game->GetSceneManager()->DuplicateObject(static_cast<Engine::Object*>(selectedObject));
				std::string s = std::to_string(obj->GetID());
				objectsName.push_back(s);
				objectsNameIDs.push_back(Engine::SID(s));
				ImGui::CloseCurrentPopup();
			}*/

			ImGui::EndPopup();
		}
		else
		{
			popupOpen = false;
		}

		if (shouldSelectEntity)
		{
			gizmo->SetSelectedEntity(startEntity);
			editorManager->GetObjectWindow().SetEntity(startEntity);
			shouldSelectEntity = false;
		}
	}
	ImGui::EndDock();
}

void SceneWindow::Show(bool show)
{
	showWindow = show;
}

void SceneWindow::RenderEntityName(const EditorName &name)
{
	ImVec2 min, max;

	ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnDoubleClick;

	bool hasChildren = transformManager->HasChildren(name.e);
	if (hasChildren == false && transformManager->HasParent(name.e) == true)
	{
		flags |= ImGuiTreeNodeFlags_Bullet;
	}
	ImGui::PushID(imguiID);

	Engine::Entity entityCopy = name.e;		// We copy the entity because when rendering the children if we had a bone attachment name, the push back of the editor names vector will invalidate the pointers so the
											// parameter name ref will contain garbage

	if (ImGui::TreeNodeEx(name.name, flags))
	{
		min = ImGui::GetItemRectMin();
		max = ImGui::GetItemRectMax();

		ImGui::Indent();
		if (hasChildren)
		{
			Engine::Entity e = transformManager->GetFirstChild(name.e);
			while (e.IsValid())
			{
				if (editorManager->GetEditorNameManager().HasName(e) == false)
					editorManager->GetEditorNameManager().SetName(e, "Bone attachement");		// right now only bone attachment entities are added without the editor name manager knowing

				RenderEntityName(editorManager->GetEditorNameManager().GetEditorName(e));
				e = transformManager->GetNextSibling(e);
			}
		}
		ImGui::Unindent();
		ImGui::TreePop();
	}
	else
	{
		min = ImGui::GetItemRectMin();
		max = ImGui::GetItemRectMax();
	}

	ImGui::PopID();
	imguiID++;

	// Drag and drop
	hovered = ImGui::IsMouseHoveringRect(min, max);		// We calculate hovered by ourselves because ImGui::IsItemHovered() was not giving correct results

	if (hovered && ImGui::IsMouseClicked(0))
	{
		startEntity = entityCopy;
		shouldSelectEntity = true;
	}
	if (Engine::Input::IsMouseButtonDown(0))
	{
		dragDelta = ImGui::GetMouseDragDelta();
	}
	if (ImGui::IsMouseReleased(0) && hovered && startEntity.id != entityCopy.id && glm::abs(dragDelta.x) > 1.0f && glm::abs(dragDelta.y) > 2.0f && !popupOpen)
	{
		transformManager->SetParent(startEntity, entityCopy);
	}

	if (Engine::Input::WasMouseButtonReleased(1) && hovered)
	{
		sprintf(popupID, "%u", entityCopy.id);		// use the entity id as an id for the popup
		selectedEntity = entityCopy;
		openPopup = true;
	}
}
