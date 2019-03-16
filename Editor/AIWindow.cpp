#include "AIWindow.h"

#include "Engine\Game\Game.h"
#include "Engine\Program\Input.h"
#include "Engine\Program\Utils.h"
#include "EditorManager.h"

#include <include\glm\gtc\type_ptr.hpp>

#include "imgui\imgui.h"
#include "imgui\imgui_dock.h"

#include <iostream>

AIWindow::AIWindow()
{
	gridCenter = glm::vec2(0.0f);
}

void AIWindow::Update()
{
	gridCenter = game->GetAISystem().GetAStarGrid().GetGridCenter();
}

void AIWindow::Render()
{
	if (BeginWindow("AI Settings"))
	{
		if (ImGui::DragFloat2("Grid center", glm::value_ptr(gridCenter), 0.1f))
		{
			game->GetAISystem().GetAStarGrid().SetGridCenter(gridCenter);
		}

		if (ImGui::Button("Build Grid"))
		{
			// Calling set layer again causes the objects to be updated in the physics world
			/*const std::vector<Engine::Object*> &objects = game->GetSceneManager()->GetObjects();
			for (size_t i = 0; i < objects.size(); i++)
			{
				if (objects[i]->GetLayer() == Engine::Layer::OBSTACLE)
					objects[i]->SetLayer(Engine::Layer::OBSTACLE);
			}*/
			//game->GetAISystem().GetAStarGrid().RebuildGrid(game, gridCenter);
			game->GetAISystem().GetAStarGrid().SetNeedsRebuild(true);
		}

		if (ImGui::Button(("Build Grid Immediate")))
		{
			// Calling set layer again causes the objects to be updated in the physics world
			/*const std::vector<Engine::Object*> &objects = game->GetSceneManager()->GetObjects();
			for (size_t i = 0; i < objects.size(); i++)
			{
				if (objects[i]->GetLayer() == Engine::Layer::OBSTACLE)
					objects[i]->SetLayer(Engine::Layer::OBSTACLE);
			}*/
			game->GetAISystem().GetAStarGrid().RebuildImmediate(game, gridCenter);
		}

		if (ImGui::Button("Show grid"))
		{
			game->GetAISystem().SetShowGrid(!game->GetAISystem().GetShowGrid());
		}

		/*if (ImGui::DragInt("Nodes rebuild per frame", &nodesRebuiltPerFrame, 0.1f))
		{
			game->GetAISystem().GetAStarGrid().SetNodesRebuildPerFrame(nodesRebuiltPerFrame);
		}*/

		if (ImGui::Button("Save grid to file"))
		{
			game->GetAISystem().GetAStarGrid().SaveGridToFile();
		}

		ImGui::Checkbox("Select grid mode", &selectGrid);

		if (selectGrid && Engine::Input::IsMousePressed(0) && editorManager->IsMouseInsideGameView())
		{
			glm::vec3 dir = Engine::utils::GetRayDirection(Engine::Input::GetMousePosition(), game->GetMainCamera());
			std::cout << dir.x << "   " << dir.y << "   " << dir.z << '\n';
			intersected = game->GetTerrain()->IntersectTerrain(game->GetMainCamera()->GetPosition(), dir, intersectionPoint);
		}

		if (intersected)
		{
			Engine::AStarNode *n = game->GetAISystem().GetAStarGrid().NodeFromWorldPos(glm::vec2(intersectionPoint.x, intersectionPoint.z));
			if (n)
			{
				walkable = n->walkable;
				if (ImGui::Checkbox("Walkable", &walkable))
				{
					game->GetAISystem().GetAStarGrid().UpdateNode(glm::vec2(intersectionPoint.x, intersectionPoint.z), walkable);
				}
			}
		}
	}
	EndWindow();
}
