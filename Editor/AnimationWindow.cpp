#include "AnimationWindow.h"

#include "Game/Game.h"
#include "EditorManager.h"
#include "Program/Input.h"

#include "imgui/imgui_dock.h"
#include "imgui/imconfig.h"

#include <iostream>

AnimationWindow::AnimationWindow()
{
	curAnimController = nullptr;

	ResetName();
}

void AnimationWindow::Render()
{
	if (BeginWindow("Animation Window"))
	{
		curEntity = editorManager->GetObjectWindow().GetSelectedEntity();

		const std::map<unsigned int, Engine::Animation*> &animations = game->GetModelManager().GetAnimations();

		if (animNames.size() < animations.size())
		{
			animNames.resize(animations.size());
			animIDs.resize(animations.size());

			unsigned int i = 0;
			for (auto it = animations.begin(); it != animations.end(); it++)
			{
				animNames[i] = it->second->name;
				animIDs[i] = it->first;

				i++;
			}
		}

		// Render the links first so they render below the nodes
		for (size_t i = 0; i < editorNodes.size(); i++)
		{
			std::vector<EditorLink> &links = editorNodes[i].GetFromLinks();
			for (size_t j = 0; j < links.size(); j++)
			{
				links[j].Render();
			}		
		}

		// Render nodes
		for (size_t i = 0; i < editorNodes.size(); i++)
		{
			ImU32 col = IM_COL32(128, 128, 128, 255);
			if (i == 0)
				col = IM_COL32(45, 190, 45, 255);
			else if (curEntity.IsValid())
			{
				Engine::AnimatedModel *am = game->GetModelManager().GetAnimatedModel(curEntity);

				if (am && am->GetAnimationController() && i == am->GetAnimationController()->GetCurrentNodeID())
					col = IM_COL32(255, 175, 20, 255);
			}

			editorNodes[i].Render(col);
		}

		HandleNodeCreation();
		HandleLinkIntersection();
		HandleLinkPositioning();
		HandleNodeIntersection();
		HandleNodeDrag();
		HandleNodeContextPopup();	
	}
	EndWindow();
}

void AnimationWindow::OpenAnimationController(const std::string &path)
{
	if (curAnimController)
	{
		delete curAnimController;
		curAnimController = nullptr;
	}

	curAnimController = new Engine::AnimationController(path);

	Engine::Serializer s(game->GetFileManager());
	s.OpenForReading(path);
	curAnimController->Deserialize(s, &game->GetModelManager());
	s.Close();

	LoadEditorAnimationController(path + "editor");		// Add editor to the extension

	isControllerCreated = true;
}

void AnimationWindow::HandleNodeCreation()
{
	ImVec2 windowPos = ImGui::GetWindowPos();
	ImVec2 windowSize = ImGui::GetWindowSize();
	ImVec2 mousePos = ImGui::GetMousePos();

	if (mousePos.x > windowPos.x && mousePos.x < (windowPos.x + windowSize.x) && mousePos.y > windowPos.y && mousePos.y < (windowPos.y + windowSize.y))
	{
		if (Engine::Input::WasMouseButtonReleased(1))
		{
			ImGui::OpenPopup("Info");
		}
	}

	if (ImGui::BeginPopup("Info"))
	{
		if (isControllerCreated)
		{
			if (ImGui::Button("Add Node"))
			{
				EditorAnimNode editorNode;
				ImVec2 toCenter = editorNode.GetPos();
				ImVec2 size = editorNode.GetSize();
				toCenter.x += size.x * 0.5f;
				toCenter.y += size.y * 0.5f;

				// If we still have 0 links then connect the node immediately to the entry node
				std::vector<Engine::AnimNode> &animNodes = curAnimController->GetAnimNodes();
				if (animNodes[0].GetLinks().size() == 0)
				{
					EditorLink editorLink;

					ImVec2 fromCenter = editorNodes[0].GetPos();
					ImVec2 size = editorNodes[0].GetSize();
					fromCenter.x += size.x * 0.5f;
					fromCenter.y += size.y * 0.5f;

					editorLink.SetFromNodePos(fromCenter, 0);
					editorLink.SetToNodePos(toCenter, 1);

					Engine::Link link;
					link.SetToNodeID(1);

					animNodes[0].AddLink(link);
					editorNodes[0].AddLink(editorLink);
				}

				editorNodes.push_back(editorNode);

				// Add the actual node to the anim controller
				Engine::AnimNode animNode;
				animNode.SetName("New node");
				animNodes.push_back(animNode);

				ImGui::CloseCurrentPopup();
			}
		}

		bool saveCurrent = false;
		bool closeOuterPopup = false;

		if (ImGui::Button("New animation controller"))
		{
			ImGui::OpenPopup("New controller");
			saveCurrent = true;
		}

		if (ImGui::Button("Load animation controller"))
		{
			saveCurrent = true;

			std::string dir = editorManager->GetCurrentProjectDir() + "/*";
			files.clear();
			Engine::utils::FindFilesInDirectory(files, dir, ".animcontroller");

			ImGui::OpenPopup("Load controller");
		}

		if (isControllerCreated && ImGui::Button("Save animation controller"))
			ImGui::OpenPopup("Save controller");

		// Save the current controller if we create a new one or load another one
		if (saveCurrent && curAnimController)
		{
			Engine::Serializer s(game->GetFileManager());
			s.OpenForWriting();
			curAnimController->Serialize(s);
			s.Save(curAnimContPath);
			s.Close();

			// Save the editor version of the controller
			SaveEditorAnimationController(curAnimContPath + "editor");		// Add editor to the extension
		}


		if (ImGui::BeginPopup("New controller"))
		{
			ResetName();

			if (ImGui::InputText("Name", name, 128, ImGuiInputTextFlags_EnterReturnsTrue))
			{
				curAnimContPath = editorManager->GetCurrentProjectDir() + '/' + name + ".animcontroller";

				if (curAnimController)
				{
					delete curAnimController;
					curAnimController = nullptr;
				}

				curAnimController = new Engine::AnimationController(curAnimContPath);

				Engine::Serializer s(game->GetFileManager());
				s.OpenForWriting();
				curAnimController->Serialize(s);
				s.Save(curAnimContPath);
				s.Close();

				// Create the editor entry node
				EditorAnimNode node;
				node.SetName(curAnimController->GetAnimNodes()[0].GetName());
				editorNodes.push_back(node);

				// Save the editor controller
				SaveEditorAnimationController(curAnimContPath + "editor");

				isControllerCreated = true;
				isControllerLoaded = true;
				closeOuterPopup = true;

				ImGui::CloseCurrentPopup();
			}
			ImGui::EndPopup();
		}


		if (ImGui::BeginPopup("Load controller"))
		{
			if (files.size() > 0)
			{
				for (size_t i = 0; i < files.size(); i++)
				{
					if (ImGui::Selectable(files[i].c_str()))
					{
						OpenAnimationController(files[i]);
						closeOuterPopup = true;
					}
				}
			}
			else
				ImGui::Text("No files found on project folder.");

			ImGui::EndPopup();
		}


		
		if (ImGui::BeginPopup("Save controller"))
		{
			ResetName();

			if (ImGui::InputText("Name", name, 128, ImGuiInputTextFlags_EnterReturnsTrue))
			{
				Engine::Serializer s(game->GetFileManager());
				s.OpenForWriting();
				curAnimController->Serialize(s);

				std::string path = editorManager->GetCurrentProjectDir() + '/' + name;

				s.Save(path + ".animcontroller");
				s.Close();

				// Save separately the position, size, etc of the nodes and links because it's just needed for the editor
				SaveEditorAnimationController(path + ".animcontrollereditor");

				// If we have an entity selected with an animated model then reload it's animation controller
				if (curEntity.IsValid())
				{
					Engine::AnimatedModel *am = game->GetModelManager().GetAnimatedModel(curEntity);
					if (am)
						am->SetAnimationController(game->GetFileManager(), path + ".animcontroller", &game->GetModelManager());
				}

				closeOuterPopup = true;
				ImGui::CloseCurrentPopup();
			}
			ImGui::EndPopup();
		}

		if (closeOuterPopup)				// Close the outer popup (new, load, save)
			ImGui::CloseCurrentPopup();

		ImGui::EndPopup();
	}
}

void AnimationWindow::HandleLinkIntersection()
{
	ImVec2 windowPos = ImGui::GetWindowPos();
	ImVec2 windowSize = ImGui::GetWindowSize();
	ImVec2 mousePos = ImGui::GetMousePos();

	if (!showingNodeContext)
	{
		if (mousePos.x > windowPos.x && mousePos.x < (windowPos.x + windowSize.x) && mousePos.y > windowPos.y && mousePos.y < (windowPos.y + windowSize.y))
		{
			if (Engine::Input::WasMouseButtonReleased(0))
			{
				currentLinkID = -1;

				for (size_t i = 0; i < editorNodes.size(); i++)
				{
					const std::vector<EditorLink> &links = editorNodes[i].GetFromLinks();
					for (size_t j = 0; j < links.size(); j++)
					{
						if (links[j].CheckIntersection())
						{
							currentLinkID = j;
							currentNodeID = i;
							std::cout << "intersected link : " << j << '\n';
							break;
						}
					}
				}
			}
		}
	}
}

void AnimationWindow::HandleLinkPositioning()
{
	if (creatingLink && currentLinkID != -1)
	{
		ImVec2 mousePos = ImGui::GetMousePos();

		EditorLink &currentLink = editorNodes[currentNodeID].GetFromLinks()[currentLinkID];

		currentLink.SetToNodePos(mousePos, 0);		// Use 0 as id because it's temporary

		// Check if we intersected any node on mouse button release
		if (Engine::Input::WasMouseButtonReleased(0))
		{
			bool intersected = false;
			bool removeLink = false;
			unsigned int i = 0;

			for (i = 0; i < editorNodes.size(); i++)
			{
				if (editorNodes[i].ContainsPoint(mousePos))
				{
					intersected = true;
					break;
				}
			}
			if (intersected)
			{
				ImVec2 center = editorNodes[i].GetPos();
				ImVec2 size = editorNodes[i].GetSize();
				center.x += size.x * 0.5f;
				center.y += size.y * 0.5f;

				currentLink.SetToNodePos(center, i);

				std::vector<EditorLink> &fromNodeLinks = editorNodes[i].GetFromLinks();
				for (size_t j = 0; j < fromNodeLinks.size(); j++)
				{
					if (currentNodeID == fromNodeLinks[j].GetToID())
					{
						currentLink.SetOffset(7.0f);
						fromNodeLinks[j].SetOffset(7.0f);
						std::cout << "igual\n";
						break;
					}
				}

				curAnimController->GetAnimNodes()[currentNodeID].GetLinks()[currentLinkID].SetToNodeID(i);

				// Check if we already have a link going to the same node
				const std::vector<EditorLink> &currentNodeLinks = editorNodes[currentNodeID].GetFromLinks();
				unsigned short sameNodeCount = 0;
				for (size_t j = 0; j < currentNodeLinks.size(); j++)
				{
					if (currentNodeLinks[j].GetToID() == i)
						sameNodeCount++;

					if (sameNodeCount == 2)
					{
						removeLink = true;
						std::cout << "removed duplicate\n";
						break;
					}
				}


				creatingLink = false;
			}
			else
			{
				removeLink = true;				
			}

			if (removeLink)
			{
				creatingLink = false;
				std::vector<EditorLink> &links = editorNodes[currentNodeID].GetFromLinks();
				links.erase(links.begin() + currentLinkID);

				curAnimController->GetAnimNodes()[currentNodeID].RemoveLink(currentLinkID);

				currentLinkID = -1;
			}
		}
	}
}

void AnimationWindow::HandleNodeIntersection()
{
	ImVec2 mousePos = ImGui::GetMousePos();

	if (!isDragging && !creatingLink)
	{
		for (size_t i = 0; i < editorNodes.size(); i++)
		{
			if (Engine::Input::IsMouseButtonDown(0))
			{
				if (editorNodes[i].ContainsPoint(mousePos))
				{
					isDragging = true;
					currentNodeID = i;
					currentLinkID = -1;
					break;
				}
			}
			else if (Engine::Input::WasMouseButtonReleased(1))
			{
				if (editorNodes[i].ContainsPoint(mousePos))
				{
					ImGui::OpenPopup("Node context");
					currentNodeID = i;
					currentLinkID = -1;				// Deselect the link (the node might not have any links which would cause a crash)
					break;
				}
			}
		}
	}
}

void AnimationWindow::HandleNodeDrag()
{
	ImVec2 mousePos = ImGui::GetMousePos();

	if (isDragging)
	{
		if (firstClick)
		{
			oldMousePos = mousePos;
			firstClick = false;
		}

		ImVec2 offset = ImVec2(mousePos.x - oldMousePos.x, mousePos.y - oldMousePos.y);
		const ImVec2 &oldPos = editorNodes[currentNodeID].GetPos();
		ImVec2 newPos = ImVec2(oldPos.x + offset.x, oldPos.y + offset.y);

		editorNodes[currentNodeID].SetPos(newPos);

		ImVec2 size = editorNodes[currentNodeID].GetSize();
		ImVec2 center = ImVec2(newPos.x + size.x * 0.5f, newPos.y + size.y * 0.5f);

		oldMousePos = mousePos;

		// Update the links positions that go out of this node
		std::vector<EditorLink> &editorLinks = editorNodes[currentNodeID].GetFromLinks();
		for (size_t i = 0; i < editorLinks.size(); i++)
		{
			editorLinks[i].SetFromNodePos(center, currentNodeID);
		}
		// And the ones that go to this node. This are a bit more tricky because a node doesn't know which links go to it
		// So we loop through every node a find out which links of a node go to the node we're moving
		for (size_t i = 0; i < editorNodes.size(); i++)
		{
			std::vector<EditorLink> &links = editorNodes[i].GetFromLinks();
			for (size_t j = 0; j < links.size(); j++)
			{
				if (links[j].GetToID() == currentNodeID)
				{
					links[j].SetToNodePos(center, currentNodeID);
				}
			}
		}
	}
}

void AnimationWindow::HandleNodeContextPopup()
{
	if (Engine::Input::WasMouseButtonReleased(0))
	{
		firstClick = true;
		isDragging = false;
	}

	if (ImGui::BeginPopup("Node context"))
	{
		showingNodeContext = true;

		std::strcpy(name, editorNodes[currentNodeID].GetName().c_str());			// Copy the current node name to the input text
		if (ImGui::InputText("Rename", name, 128, ImGuiInputTextFlags_EnterReturnsTrue))
		{	
			editorNodes[currentNodeID].SetName(name);
			curAnimController->GetAnimNodes()[currentNodeID].SetName(name);
			//ImGui::CloseCurrentPopup();
		}	

		if (ImGui::Button("Create link"))
		{
			creatingLink = true;

			ImVec2 center = editorNodes[currentNodeID].GetPos();
			ImVec2 size = editorNodes[currentNodeID].GetSize();

			center.x += size.x * 0.5f;
			center.y += size.y * 0.5f;

			EditorLink editorLink;
			editorLink.SetFromNodePos(center, currentNodeID);			// Set the center of the node as the position

			std::vector<EditorLink> &links = editorNodes[currentNodeID].GetFromLinks();
			links.push_back(editorLink);

			currentLinkID = links.size() - 1;

			Engine::Link animLink;
			curAnimController->GetAnimNodes()[currentNodeID].AddLink(animLink);

			ImGui::CloseCurrentPopup();
		}

		if (currentNodeID != 0)			// Only allow this if we don't have the entry node selected
		{
			std::vector<Engine::AnimNode> &animNodes = curAnimController->GetAnimNodes();
			looped = animNodes[currentNodeID].IsLooping();

			const std::map<unsigned int, Engine::Animation*> &animations = game->GetModelManager().GetAnimations();

			// Find the animation index for the combo box
			int i = 0;
			for (auto it = animations.begin(); it != animations.end(); it++)
			{
				if (animNodes[currentNodeID].GetAnimID() == it->first)
				{
					nodeAnimIndex = i;
					break;
				}
				
				i++;
			}

			if (ImGui::Combo("Animation", &nodeAnimIndex, animNames))
			{
				animNodes[currentNodeID].SetAnimationID(&game->GetModelManager(), (unsigned int)animIDs[nodeAnimIndex]);
			}
			if (ImGui::Checkbox("Loop", &looped))
			{
				animNodes[currentNodeID].SetLooping(looped);
			}
			if (ImGui::Button("Delete"))
			{

			}
		}

		ImGui::EndPopup();
	}
	else
	{
		showingNodeContext = false;
	}
}

void AnimationWindow::ResetName()
{
	for (size_t i = 0; i < 128; i++)
	{
		name[i] = 0;
	}
}

void AnimationWindow::SaveEditorAnimationController(const std::string &path)
{
	Engine::Serializer s(game->GetFileManager());
	s.OpenForWriting();

	s.Write(static_cast<unsigned int>(editorNodes.size()));

	for (size_t i = 0; i < editorNodes.size(); i++)
	{
		editorNodes[i].Serialize(s);
	}

	s.Save(path);
	s.Close();
}

void AnimationWindow::LoadEditorAnimationController(const std::string &path)
{
	Engine::Serializer s(game->GetFileManager());
	s.OpenForReading(path);

	unsigned int numNodes = 0;
	s.Read(numNodes);
	editorNodes.resize(numNodes);

	for (size_t i = 0; i < editorNodes.size(); i++)
	{
		editorNodes[i].Deserialize(s);
	}

	s.Close();
}

EditorAnimNode::EditorAnimNode()
{
	name = "New Node";
	ImVec2 windowPos = ImGui::GetWindowPos();
	ImVec2 windowSize = ImGui::GetWindowSize();
	pos = ImVec2(windowPos.x + windowSize.x * 0.5f, windowPos.y + windowSize.y * 0.5f);
	size = ImVec2(100.0f, 20.0f);
}

void EditorAnimNode::Render(ImU32 col)
{
	ImDrawList *drawList = ImGui::GetWindowDrawList();
	float text_width = ImGui::CalcTextSize(name.c_str()).x;

	drawList->AddRectFilled(ImVec2(pos.x, pos.y), ImVec2(pos.x + size.x, pos.y + size.y), col, 4.0f);
	//drawList->AddRect(ImVec2(pos.x, pos.y), ImVec2(pos.x + size.x, pos.y + size.y), IM_COL32(255,0,0,255), 4.0f);

	ImGui::SetCursorScreenPos(ImVec2(pos.x + (size.x - text_width) * 0.5f, pos.y + size.y * 0.125f));
	ImGui::Text(name.c_str());
	ImGui::SetCursorScreenPos(pos);
}

bool EditorAnimNode::ContainsPoint(const ImVec2 &point)
{
	if (point.x > pos.x && point.x < (pos.x + size.x) && point.y > pos.y && point.y < (pos.y + size.y))
		return true;

	return false;
}

void EditorAnimNode::Serialize(Engine::Serializer &s)
{
	glm::vec2 glmpos = glm::vec2(pos.x, pos.y);
	glm::vec2 glmsize = glm::vec2(size.x, size.y);
	s.Write(glmpos);
	s.Write(glmsize);
	s.Write(name);
	s.Write(static_cast<unsigned int>(editorLinks.size()));

	for (size_t i = 0; i < editorLinks.size(); i++)
	{
		editorLinks[i].Serialize(s);
	}
}

void EditorAnimNode::Deserialize(Engine::Serializer &s)
{
	glm::vec2 glmpos = glm::vec2(0.0f);
	glm::vec2 glmsize = glm::vec2(0.0f);
	s.Read(glmpos);
	s.Read(glmsize);

	pos = ImVec2(glmpos.x, glmpos.y);
	size = ImVec2(glmsize.x, glmsize.y);

	s.Read(name);

	unsigned int numLinks = 0;
	s.Read(numLinks);
	editorLinks.resize(numLinks);

	for (size_t i = 0; i < editorLinks.size(); i++)
	{
		editorLinks[i].Deserialize(s);
	}
}

EditorLink::EditorLink()
{
	offset = 0.0f;
}

bool EditorLink::CheckIntersection() const
{
	ImVec2 fromWithOffset = ImVec2(from.x + offset * right.x, from.y + offset * right.y);
	ImVec2 toWithOffset = ImVec2(to.x + offset * right.x, to.y + offset * right.y);

	ImVec2 dif = ImVec2(fromWithOffset.x - toWithOffset.x, fromWithOffset.y - toWithOffset.y);

	ImVec2 mousePos = ImGui::GetMousePos();

	float lenSqrd = dif.x * dif.x + dif.y * dif.y;
	ImVec2 a = ImVec2(mousePos.x - fromWithOffset.x, mousePos.y - fromWithOffset.y);
	ImVec2 b = ImVec2(toWithOffset.x - fromWithOffset.x, toWithOffset.y - fromWithOffset.y);
	float t = glm::clamp((a.x * b.x + a.y * b.y) / lenSqrd, 0.0f, 1.0f);
	const ImVec2 proj = ImVec2(fromWithOffset.x + (toWithOffset.x - fromWithOffset.x) * t, fromWithOffset.y + (toWithOffset.y - fromWithOffset.y) * t);
	ImVec2 dist = ImVec2(mousePos.x - proj.x, mousePos.y - proj.y);

	return (dist.x * dist.x + dist.y * dist.y) < 100;
}

void EditorLink::Render()
{
	ImDrawList *drawList = ImGui::GetWindowDrawList();

	ImVec2 halfway = ImVec2(to.x - from.x, to.y - from.y);
	halfway.x *= 0.5f;
	halfway.y *= 0.5f;
	float length = sqrt(halfway.x * halfway.x + halfway.y * halfway.y);

	ImVec2 dir = ImVec2(to.x - from.x, to.y - from.y);
	float x = (1 / sqrt(dir.x * dir.x + dir.y * dir.y));
	dir.x *= x;
	dir.y *= x;

	right.x = dir.y;
	right.y = -dir.x;
	
	drawList->AddLine(ImVec2(from.x + offset * right.x, from.y + offset * right.y), ImVec2(to.x + offset * right.x, to.y + offset * right.y), IM_COL32(255, 255, 255, 255), 2.0f);

	ImVec2 half = ImVec2(from.x + dir.x * length, from.y + dir.y * length);

	drawList->AddLine(ImVec2(half.x + offset * right.x, half.y + offset * right.y), ImVec2(half.x - dir.x * 10.0f + right.x * 5.0f + offset * right.x, half.y - dir.y * 10.0f + right.y * 5.0f + offset * right.y), IM_COL32(255, 255, 255, 255), 2.0f);
	drawList->AddLine(ImVec2(half.x + offset * right.x, half.y + offset * right.y), ImVec2(half.x - dir.x * 10.0f - right.x * 5.0f + offset * right.x, half.y - dir.y * 10.0f - right.y * 5.0f + offset * right.y), IM_COL32(255, 255, 255, 255), 2.0f);

}

void EditorLink::Serialize(Engine::Serializer &s)
{
	s.Write(fromID);
	s.Write(toID);

	glm::vec2 glmfrom = glm::vec2(from.x, from.y);
	glm::vec2 glmto = glm::vec2(to.x, to.y);
	s.Write(glmfrom);
	s.Write(glmto);
	s.Write(offset);
}

void EditorLink::Deserialize(Engine::Serializer &s)
{
	s.Read(fromID);
	s.Read(toID);

	glm::vec2 glmfrom = glm::vec2(0.0f);
	glm::vec2 glmto = glm::vec2(0.0f);
	s.Read(glmfrom);
	s.Read(glmto);

	from = ImVec2(glmfrom.x, glmfrom.y);
	to = ImVec2(glmto.x, glmto.y);

	s.Read(offset);
}
