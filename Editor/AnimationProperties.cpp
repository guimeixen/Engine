#include "AnimationProperties.h"

#include "EditorManager.h"

#include "imgui\imgui.h"
#include "imgui\imgui_dock.h"
#include "imgui\imconfig.h"

AnimationProperties::AnimationProperties()
{
	game = nullptr;
	editorManager = nullptr;
	curAnimController = nullptr;
}

void AnimationProperties::Init(Engine::Game *game, EditorManager *editorManager)
{
	this->game = game;
	this->editorManager = editorManager;
}

void AnimationProperties::Render()
{
	if (ImGui::BeginDock("Animation Properties", &showWindow))
	{
		curEntity = editorManager->GetObjectWindow().GetSelectedEntity();
		curAnimController = editorManager->GetAnimationWindow().GetCurrentAnimationController();

		if (!curAnimController)
		{
			ImGui::EndDock();
			return;
		}

		std::vector<Engine::ParameterDesc> &parameters = curAnimController->GetParameters();

		ImGui::Text("Parameters");

		int j = 100;			// To use as id for imgui
		for (size_t i = 0; i < parameters.size(); i++)
		{
			Engine::ParameterDesc &pd = parameters[i];

			if (paramNames.size() <= i)
			{
				char name[128];
				strcpy_s(name, 128, pd.name.c_str());
				paramNames.push_back(name);

				if (pd.type == Engine::ParamType::INT)
				{			
					intCombos.push_back(0);
					inputInts.push_back(0);
				}
				else if (pd.type == Engine::ParamType::FLOAT)
				{
					floatCombos.push_back(0);
					inputFloats.push_back(0.0f);
				}
				else if (pd.type == Engine::ParamType::BOOL)
				{
					boolCombos.push_back(0);
				}
			}

			ImGui::PushID(i);
			ImGui::PushItemWidth(150);
			if (ImGui::InputText("Name", paramNames[i], 128, ImGuiInputTextFlags_EnterReturnsTrue))
			{
				pd.name = paramNames[i];
			}
			ImGui::PopID();
			ImGui::PopItemWidth();
			ImGui::SameLine();
			
			ImGui::PushID(j);

			// Replace with entity
			if (pd.type == Engine::ParamType::INT)
			{
				ImGui::PushItemWidth(150);
				ImGui::DragInt("", &pd.param.intVal);
				ImGui::PopItemWidth();

				if (curEntity.IsValid())
				{
					Engine::AnimatedModel *am = game->GetModelManager().GetAnimatedModel(curEntity);
					if (am && am->GetAnimationController())
						am->GetAnimationController()->GetParameters()[i].param.intVal = pd.param.intVal;
				}
			}
			else if (pd.type == Engine::ParamType::FLOAT)
			{
				ImGui::PushItemWidth(150);
				ImGui::DragFloat("", &pd.param.floatVal, 0.1f);
				ImGui::PopItemWidth();

				if (curEntity.IsValid())
				{
					Engine::AnimatedModel *am = game->GetModelManager().GetAnimatedModel(curEntity);
					if (am && am->GetAnimationController())
						am->GetAnimationController()->GetParameters()[i].param.floatVal = pd.param.floatVal;
				}
			}
			else if (pd.type == Engine::ParamType::BOOL)
			{
				ImGui::Checkbox("", &pd.param.boolVal);

				if (curEntity.IsValid())
				{
					Engine::AnimatedModel *am = game->GetModelManager().GetAnimatedModel(curEntity);
					if (am && am->GetAnimationController())
						am->GetAnimationController()->GetParameters()[i].param.boolVal = pd.param.boolVal;
				}
			}
			
			ImGui::PopID();

			j++;
		}

		HandleAddParameter(parameters);
		ImGui::Separator();
		HandleLinkOptions(parameters);
	}
	ImGui::EndDock();
}

void AnimationProperties::HandleAddParameter(std::vector<Engine::ParameterDesc> &parameters)
{
	if (ImGui::Button("Add Parameter"))
	{
		ImGui::OpenPopup("add param popup");
	}
	if (ImGui::BeginPopup("add param popup"))
	{
		if (ImGui::Button("Float parameter"))
		{
			Engine::ParameterDesc param = {};
			param.param.floatVal = 0.0f;
			param.type = Engine::ParamType::FLOAT;
			param.name = "New float parameter";

			parameters.push_back(param);

			if (curEntity.IsValid())
			{
				Engine::AnimatedModel *am = game->GetModelManager().GetAnimatedModel(curEntity);
				if (am && am->GetAnimationController())
					am->GetAnimationController()->GetParameters().push_back(param);
			}

			floatCombos.push_back(0);
			inputFloats.push_back(0.0f);

			char name[128];
			std::memset(name, 0, 128);
			paramNames.push_back(name);

			ImGui::CloseCurrentPopup();
		}
		if (ImGui::Button("Int parameter"))
		{
			Engine::ParameterDesc param = {};
			param.param.intVal = 0;
			param.type = Engine::ParamType::INT;
			param.name = "New int parameter";

			parameters.push_back(param);

			if (curEntity.IsValid())
			{
				Engine::AnimatedModel *am = game->GetModelManager().GetAnimatedModel(curEntity);
				if (am && am->GetAnimationController())
					am->GetAnimationController()->GetParameters().push_back(param);
			}

			intCombos.push_back(0);
			inputInts.push_back(0);

			char name[128];
			std::memset(name, 0, 128);
			paramNames.push_back(name);

			ImGui::CloseCurrentPopup();
		}
		if (ImGui::Button("Bool parameter"))
		{
			Engine::ParameterDesc param = {};
			param.param.boolVal = false;
			param.type = Engine::ParamType::BOOL;
			param.name = "New bool parameter";

			parameters.push_back(param);

			if (curEntity.IsValid())
			{
				Engine::AnimatedModel *am = game->GetModelManager().GetAnimatedModel(curEntity);
				if (am && am->GetAnimationController())
					am->GetAnimationController()->GetParameters().push_back(param);
			}

			boolCombos.push_back(0);

			char name[128];
			std::memset(name, 0, 128);
			paramNames.push_back(name);

			ImGui::CloseCurrentPopup();
		}

		ImGui::EndPopup();
	}
}

void AnimationProperties::HandleLinkOptions(const std::vector<Engine::ParameterDesc> &parameters)
{
	ImGui::Text("Link options");

	AnimationWindow &animWindow = editorManager->GetAnimationWindow();

	if (animWindow.GetCurrentLinkID() != -1)
	{
		std::vector<Engine::AnimNode> &animNodes = curAnimController->GetAnimNodes();
		std::vector<Engine::Link> &links = animNodes[animWindow.GetCurrentNodeID()].GetLinks();

		Engine::Link &currentLink = links[animWindow.GetCurrentLinkID()];

		if (currentLink.GetToNodeID() >= 0)
		{
			std::string linkTransitionName = animNodes[animWindow.GetCurrentNodeID()].GetName();
			linkTransitionName += " -> ";
			linkTransitionName += animNodes[currentLink.GetToNodeID()].GetName();

			ImGui::Text(linkTransitionName.c_str());
		}

		transitionTime = currentLink.GetTransitionTime();
		if (ImGui::InputFloat("Transition time", &transitionTime, 0.01f))
		{
			currentLink.SetTransitionTime(transitionTime);
		}

		if (ImGui::Button("Choose parameter"))
			ImGui::OpenPopup("Choose parameter popup");

		int paramID = -1;

		if (ImGui::BeginPopup("Choose parameter popup"))
		{
			for (size_t i = 0; i < parameters.size(); i++)
			{
				if (ImGui::Selectable(parameters[i].name.c_str()))
				{
					paramID = i;
					currentLink.AddParamID(static_cast<unsigned int>(i));
					break;
				}
			}
			ImGui::EndPopup();
		}

		const std::vector<Engine::Condition> &conditions = currentLink.GetConditions();
		int j = 1000;

		for (size_t i = 0; i < conditions.size(); i++)
		{
			//const Engine::Condition &condition = conditions[i];
			const Engine::ParameterDesc &param = parameters[conditions[i].paramID];

			int curIntCombo = 0;
			int curFloatCombo = 0;
			int curBoolCombo = 0;


			ImGui::Text(param.name.c_str());
			ImGui::SameLine();

			if (param.type == Engine::ParamType::INT)
			{
				inputInts[curIntCombo] = conditions[i].param.intVal;

				ImGui::PushID(i);
				if (ImGui::Combo("", &intCombos[curIntCombo], "greater\0less\0equal\0not equal"))
				{
					if (intCombos[curIntCombo] == 0)
						currentLink.SetCondition(i, Engine::TransitionCondition::GREATER);
					else if (intCombos[curIntCombo] == 1)
						currentLink.SetCondition(i, Engine::TransitionCondition::LESS);
					else if (intCombos[curIntCombo] == 2)
						currentLink.SetCondition(i, Engine::TransitionCondition::EQUAL);
					else if (intCombos[curIntCombo] == 3)
						currentLink.SetCondition(i, Engine::TransitionCondition::NOT_EQUAL);
				}
				ImGui::PopID();
				ImGui::SameLine();

				ImGui::PushID(j);
				ImGui::PushItemWidth(150);
				if (ImGui::DragInt("", &inputInts[curIntCombo]))
				{
					currentLink.SetConditionInt(i, inputInts[curIntCombo]);
				}
				ImGui::PopItemWidth();
				ImGui::PopID();

				curIntCombo++;
			}
			else if (param.type == Engine::ParamType::FLOAT)
			{
				inputFloats[curFloatCombo] = conditions[i].param.floatVal;
				if (conditions[i].condition == Engine::TransitionCondition::GREATER)
					floatCombos[curFloatCombo] = 0;
				else if (conditions[i].condition == Engine::TransitionCondition::LESS)
					floatCombos[curFloatCombo] = 1;

				ImGui::PushID(i);
				if (ImGui::Combo("", &floatCombos[curFloatCombo], "greater\0less"))
				{
					if (floatCombos[curFloatCombo] == 0)
						currentLink.SetCondition(i, Engine::TransitionCondition::GREATER);
					else if (floatCombos[curFloatCombo] == 1)
						currentLink.SetCondition(i, Engine::TransitionCondition::LESS);
				}
				ImGui::PopID();
				ImGui::SameLine();

				ImGui::PushID(j);
				ImGui::PushItemWidth(150);
				if (ImGui::DragFloat("", &inputFloats[curFloatCombo], 0.1f))
				{
					currentLink.SetConditionFloat(i, inputFloats[curFloatCombo]);
				}
				ImGui::PopItemWidth();
				ImGui::PopID();

				curFloatCombo++;
			}
			else if (param.type == Engine::ParamType::BOOL)
			{
				boolCombos[curBoolCombo] = conditions[i].param.boolVal;

				ImGui::PushID(i);
				if (ImGui::Combo("", &boolCombos[curBoolCombo], "false\0true"))
				{
					if (boolCombos[curBoolCombo] == 0)
					{
						currentLink.SetCondition(i, Engine::TransitionCondition::NOT_EQUAL);
						currentLink.SetConditionBool(i, false);
					}
					else if (boolCombos[curBoolCombo] == 1)
					{
						currentLink.SetCondition(i, Engine::TransitionCondition::EQUAL);
						currentLink.SetConditionBool(i, true);
					}
				}
				ImGui::PopID();
				curBoolCombo++;
			}
			j++;
		}

	}
}
