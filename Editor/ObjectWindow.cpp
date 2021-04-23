#include "ObjectWindow.h"

#include "Game/Game.h"
#include "Game/UI/Button.h"
#include "Game/UI/StaticText.h"
#include "Game/UI/Image.h"
#include "Game/UI/EditText.h"

#include "Graphics/ParticleSystem.h"
#include "Graphics/Model.h"
#include "Graphics/Material.h"
#include "Graphics/Animation/AnimatedModel.h"
#include "Graphics/Renderer.h"
#include "Graphics/VertexArray.h"
#include "Graphics/Effects/MainView.h"

#include "Sound/SoundSource.h"

#include "Physics/Trigger.h"
#include "Physics/RigidBody.h"
#include "Physics/Collider.h"

#include "Program/Input.h"
#include "Program/Utils.h"

#include "AI/AIObject.h"

#include "EditorManager.h"
#include "AssimpLoader.h"

#include <include/glm/gtc/matrix_transform.hpp>
#include <include/glm/gtc/type_ptr.hpp>

#include "imgui/imgui.h"
#include "imgui/imgui_dock.h"
#include "imgui/imconfig.h"

#include <iostream>
#include <filesystem>

#include <Windows.h>

ObjectWindow::ObjectWindow()
{
	selectedModel = nullptr;
	selectedPS = nullptr;
	selectedRB = nullptr;
	selectedTrigger = nullptr;
	selectedCollider = nullptr;
	selectedScript = nullptr;
	selectedLight = nullptr;
	selectedSoundSource = nullptr;
	selectedWidget = nullptr;
	position = glm::vec3();
	rotation = glm::vec3();
	scale = glm::vec3(1.0f);

	colliderCenter = glm::vec3(0.0f);

	isSelectingAsset = false;

	addModelSelected = false;
	addAnimModelSelected = false;
	addScriptSelected = false;

	modelOpen = true;
	rigidBodyOpen = true;
	particleSystemOpen = true;
	triggerOpen = true;
	scriptOpen = true;
	colliderOpen = true;
	lightOpen = true;
	soundSourceOpen = true;
	widgetOpen = true;

	psColor = glm::vec4(1.0f);

	castShadows = false;
	lodDistance = 0.0f;
}

void ObjectWindow::Init(Engine::Game *game, EditorManager *editorManager)
{
	EditorWindow::Init(game, editorManager);

	transformManager = &game->GetTransformManager();
	modelManager = &game->GetModelManager();
	particleManager = &game->GetParticleManager();
	physicsManager = &game->GetPhysicsManager();

	memset(scriptNameInputBuffer, 0, 64);
}

void ObjectWindow::Render()
{
	if (BeginWindow("Object Window"))
	{
		isSelectingAsset = false;

		if (!selected)
		{
			EndWindow();
			return;
		}	

		if (Engine::Input::WasKeyReleased(Engine::Keys::KEY_DEL))
		{
			game->GetEntityManager().Destroy(selectedEntity);
			editorManager->GetGizmo().DeselectEntity();
			DeselectEntity();
			EndWindow();
			return;
		}

		ImGui::Text(editorManager->GetEditorNameManager().GetName(selectedEntity));

		//Engine::ObjectType type = obj->GetType();

		/*if (type == Engine::ObjectType::DEFAULT_OBJECT)
		{
			ImGui::Text("Object type: Default");
		}
		else if (type == Engine::ObjectType::AI_OBJECT)
		{
			ImGui::Text("Object type: AI");
		}*/

		/*const char* items[] = { "Default", "Obstacle", "Static", "Enemy" };

		if (ImGui::Combo("Layer", &layerComboID, items, 4))
		{
			switch (layerComboID)
			{
			case 0:
				obj->SetLayer(Engine::Layer::DEFAULT);
				break;
			case 1:
				obj->SetLayer(Engine::Layer::OBSTACLE);
				break;
			case 2:
				obj->SetLayer(Engine::Layer::STATIC);
				break;
			case 3:
				obj->SetLayer(Engine::Layer::ENEMY);
				break;
			}
		}*/

		if (isEntityEnabled)
		{
			if (ImGui::Button("Disable"))
			{
				game->SetEntityEnabled(selectedEntity, false);
				isEntityEnabled = false;
			}
		}
		else
		{
			if (ImGui::Button("Enable"))
			{
				game->SetEntityEnabled(selectedEntity, true);
				isEntityEnabled = true;
			}
		}
		
		ImGui::SameLine();
		if (ImGui::Button("Duplicate") || (Engine::Input::IsKeyPressed(Engine::Keys::KEY_LEFT_CONTROL) && Engine::Input::WasKeyReleased(Engine::Keys::KEY_D)))
		{
			Engine::Entity newE = game->DuplicateEntity(selectedEntity);
			editorManager->GetGizmo().SetSelectedEntity(newE);
			SetEntity(newE);
		}
		ImGui::SameLine();
		if (ImGui::Button("Save Prefab"))
		{
			if (CreatePrefabFolder())
			{
				std::string path = editorManager->GetCurrentProjectDir() + "/Prefabs/" + editorManager->GetEditorNameManager().GetName(selectedEntity) + ".prefab";
				game->SaveEntityPrefab(selectedEntity, path);
			}
		}

		/*if (type == Engine::ObjectType::AI_OBJECT)
		{
			HandleAI();
		}*/

		HandleTransform();
		HandleModel();
		HandleRigidBody();
		HandleParticleSystem();
		HandleTrigger();
		HandleScript();
		HandleCollider();
		HandleLight();
		HandleSoundSource();
		HandleWidget();

		ImGui::Separator();

		// Pop up to add another component
		if (ImGui::Button("Add Component"))
		{
			ImGui::OpenPopup("Add");
		}
		if (ImGui::BeginPopup("Add"))
		{
			if (ImGui::BeginMenu("Rendering"))
			{
				if (modelManager->HasModel(selectedEntity) == false)
				{
					if (ImGui::Selectable("Model"))
						addModelSelected = true;
					if (ImGui::Selectable("Animated Model"))
						addAnimModelSelected = true;
				}

				if (particleManager->HasParticleSystem(selectedEntity) == false)
					if (ImGui::Selectable("Particle System"))
						AddParticleSystem();

				if (game->GetLightManager().HasPointLight(selectedEntity) == false)
					if (ImGui::Selectable("Point light"))
						AddLight(Engine::LightType::POINT);

				//if (game->GetLightManager().HasPointLight(selectedEntity) == false)
				//	if (ImGui::Selectable("Directional light"))
				//		AddLight(Engine::LightType::DIRECTIONAL);
				//if (!obj->GetLight())
				//				if (ImGui::Selectable("Spot light"))
				//AddLight(Engine::LightType::SPOT);

				ImGui::EndMenu();
			}

			bool canAddWidget = !game->GetUIManager().HasWidget(selectedEntity);

			if (canAddWidget && ImGui::BeginMenu("UI"))
			{
				if (ImGui::Selectable("Text"))
					AddWidget(Engine::WidgetType::TEXT);
				if (ImGui::Selectable("Button"))
					AddWidget(Engine::WidgetType::BUTTON);
				if (ImGui::Selectable("Edit text"))
					AddWidget(Engine::WidgetType::EDIT_TEXT);
				if (ImGui::Selectable("Image"))
					AddWidget(Engine::WidgetType::IMAGE);

				ImGui::EndMenu();
			}

			bool canAddPhysicsComp = !physicsManager->HasRigidBody(selectedEntity) && !physicsManager->HasTrigger(selectedEntity) && !physicsManager->HasCollider(selectedEntity);

			if (canAddPhysicsComp && ImGui::BeginMenu("Physics"))
			{
				if (ImGui::Selectable("Box Rigid body"))
					AddRigidBody(Engine::ShapeType::BOX);
				if (ImGui::Selectable("Sphere Rigid body"))
					AddRigidBody(Engine::ShapeType::SPHERE);
				if (ImGui::Selectable("Capsule Rigid body"))
					AddRigidBody(Engine::ShapeType::CAPSULE);


				if (ImGui::Selectable("Box Trigger"))
					AddTrigger(Engine::ShapeType::BOX);
				if (ImGui::Selectable("Sphere Trigger"))
					AddTrigger(Engine::ShapeType::SPHERE);
				if (ImGui::Selectable("Capsule Trigger"))
					AddTrigger(Engine::ShapeType::CAPSULE);

				if (ImGui::Selectable("Box Collider"))
					AddCollider(Engine::ShapeType::BOX);
				if (ImGui::Selectable("Sphere Collider"))
					AddCollider(Engine::ShapeType::SPHERE);
				if (ImGui::Selectable("Capsule Collider"))
					AddCollider(Engine::ShapeType::CAPSULE);

				ImGui::EndMenu();
			}

			if (ImGui::BeginMenu("Sound"))
			{
				if (game->GetSoundManager().HasSoundSource(selectedEntity) == false)
					if (ImGui::Selectable("Sound Source"))
						AddSoundSource();

				ImGui::EndMenu();
			}

			if (ImGui::BeginMenu("Scripting"))
			{
				if(game->GetScriptManager().HasScript(selectedEntity) == false)
					if (ImGui::Selectable("Script"))
						addScriptSelected = true;

				ImGui::EndMenu();
			}		

			ImGui::EndPopup();

			// Open here otherwise it won't appear
			if (addModelSelected)
			{
				ImGui::OpenPopup("Choose model");
				std::string dir = editorManager->GetCurrentProjectDir() + "/*";
				files.clear();
				Engine::utils::FindFilesInDirectory(files, dir, ".obj");
				Engine::utils::FindFilesInDirectory(files, dir, ".fbx");
				Engine::utils::FindFilesInDirectory(files, dir, ".md5mesh");
				Engine::utils::FindFilesInDirectory(files, dir, ".FBX");
			}
			if (addAnimModelSelected)
			{
				ImGui::OpenPopup("Choose model");
				std::string dir = editorManager->GetCurrentProjectDir() + "/*";
				files.clear();
				Engine::utils::FindFilesInDirectory(files, dir, ".obj");
				Engine::utils::FindFilesInDirectory(files, dir, ".fbx");
				Engine::utils::FindFilesInDirectory(files, dir, ".md5mesh");
				Engine::utils::FindFilesInDirectory(files, dir, ".FBX");
			}
			if (addScriptSelected)
			{
				ImGui::OpenPopup("Choose script");
				std::string dir = editorManager->GetCurrentProjectDir() + "/*";
				files.clear();
				Engine::utils::FindFilesInDirectory(files, dir, ".lua");
			}
		}

		if (addModelSelected)
		{
			if (ImGui::BeginPopup("Choose model"))
			{
				isSelectingAsset = true;
				if (files.size() > 0)
					AddModel(false);
				else
					ImGui::Text("No models found on project folder.");

				ImGui::EndPopup();
			}
			else
				addModelSelected = false;
		}
		if (addAnimModelSelected)
		{
			if (ImGui::BeginPopup("Choose model"))
			{
				isSelectingAsset = true;
				if (files.size() > 0)
					AddModel(true);
				else
					ImGui::Text("No models found on project folder.");

				ImGui::EndPopup();
			}
			else
				addAnimModelSelected = false;
		}
		if (addScriptSelected)
		{
			if (ImGui::BeginPopup("Choose script"))
			{
				isSelectingAsset = true;
				bool openCreate = false;

				if (ImGui::Button("Create new script"))
					openCreate = true;

				if (files.size() > 0)
					AddScript();
				else
					ImGui::Text("No scripts found on project folder.");

				ImGui::EndPopup();

				if(openCreate)
					ImGui::OpenPopup("Create new script popup");
			}
			else
				addScriptSelected = false;
		}

		if (ImGui::BeginPopup("Create new script popup"))
		{
			if (ImGui::InputText("Name", scriptNameInputBuffer, 64, ImGuiInputTextFlags_EnterReturnsTrue) || ImGui::Button("Create"))
			{
				std::string path = editorManager->GetCurrentProjectDir() + '/' + scriptNameInputBuffer + ".lua";

				if (std::filesystem::exists(path))
				{
					// File already exists
					// TODO show alert
				}
				else
				{
					std::ofstream script(path);
					script << "require('common')\n\n" + std::string(scriptNameInputBuffer) + " = {\n\n\tonInit = function(self, e)\n\tend,\n\n\tonUpdate = function(self, e, dt)\n\tend\n}";
					script.close();				// close because we're going to open it in scriptmanager to load the script. (Is it necessary?)
					game->GetScriptManager().AddScript(selectedEntity, path);

					SetEntity(selectedEntity);		// Call to show the script component in the object tab

					Engine::utils::OpenFileWithDefaultProgram(path);

					memset(scriptNameInputBuffer, 0, 64);
				}			

				ImGui::CloseCurrentPopup();
			}
			ImGui::EndPopup();
		}

		//ImGui::PopItemWidth();
	}
	EndWindow();

	// If a component was removed, remove it from the object
	if (!modelOpen && selectedModel)
	{
		modelManager->RemoveModel(selectedEntity);
		selectedModel = nullptr;
		modelOpen = true;
	}
	if (!particleSystemOpen && selectedPS)
	{
		particleManager->RemoveParticleSystem(selectedEntity);
		selectedPS = nullptr;
		particleSystemOpen = true;
	}
	if (!rigidBodyOpen && selectedRB)
	{
		physicsManager->RemoveRigidBody(selectedEntity);
		selectedRB = nullptr;
		rigidBodyOpen = true;
	}
	if (!colliderOpen && selectedCollider)
	{
		physicsManager->RemoveCollider(selectedEntity);
		selectedCollider = nullptr;
		colliderOpen = true;
	}
	if (!triggerOpen && selectedTrigger)
	{
		physicsManager->RemoveTrigger(selectedEntity);
		selectedTrigger = nullptr;
		triggerOpen = true;
	}
	if (!scriptOpen && selectedScript)
	{
		game->GetScriptManager().RemoveScript(selectedEntity);
		selectedScript = nullptr;
		scriptOpen = true;
	}
	if (!lightOpen && selectedLight)
	{
		game->GetLightManager().RemoveLight(selectedEntity);
		selectedLight = nullptr;
		lightOpen = true;
	}
	if (!soundSourceOpen && selectedSoundSource)
	{
		game->GetSoundManager().RemoveSoundSource(selectedEntity);
		selectedSoundSource = nullptr;
		soundSourceOpen = true;
	}
}

/*void ObjectWindow::SetObject(Engine::Object *obj)
{
	layerComboID = obj->GetLayer();
	switch (layerComboID)
	{
	case Engine::Layer::DEFAULT:
		layerComboID = 0;
		break;
	case Engine::Layer::OBSTACLE:
		layerComboID = 1;
		break;
	case Engine::Layer::STATIC:
		layerComboID = 2;
		break;
	case Engine::Layer::ENEMY:
		layerComboID = 3;
		break;
	}

	/*if (obj->GetType() == Engine::ObjectType::AI_OBJECT)
	{
		Engine::AIObject *aiObj = static_cast<Engine::AIObject*>(obj);
		Engine::Object *target = aiObj->GetTarget();
		if (target)
			targetName = editorManager->GetSceneWindow().GetNameAt(target->GetID());
		eyesOffset = aiObj->GetEyesOffset();
		eyesRange = aiObj->GetEyesRange();
		attackRange = aiObj->GetAttackRange();
		attackDelay = aiObj->GetAttackDelay();
		fov = aiObj->GetFieldOfView();
	}
}*/

void ObjectWindow::SetEntity(Engine::Entity entity)
{
	if (entity.IsValid() == false)
	{
		DeselectEntity();
		return;
	}

	selectedEntity = entity;
	selected = true;
	isEntityEnabled = game->GetEntityManager().IsEntityEnabled(entity);

	selectedModel = nullptr;
	selectedPS = nullptr;
	selectedRB = nullptr;
	selectedTrigger = nullptr;
	selectedCollider = nullptr;
	selectedScript = nullptr;
	selectedLight = nullptr;
	selectedSoundSource = nullptr;
	selectedWidget = nullptr;

	position = transformManager->GetLocalPosition(entity);
	glm::quat rot = transformManager->GetLocalRotation(entity);
	rotation = glm::degrees(glm::eulerAngles(rot));
	scale = transformManager->GetLocalScale(entity);

	if (modelManager->HasModel(entity))
	{
		selectedModel = modelManager->GetModel(entity);
		castShadows = selectedModel->GetCastShadows();
		lodDistance = selectedModel->GetLODDistance();
	}
	if (particleManager->HasParticleSystem(entity))
	{
		selectedPS = particleManager->GetParticleSystem(entity);
		size = selectedPS->GetSize();
		emission = selectedPS->GetEmission();
		psLifetime = selectedPS->GetLifetime();
		maxParticles = selectedPS->GetMaxParticles();
		loopPs = selectedPS->IsLooping();
		duration = selectedPS->GetDuration();
		velocity = selectedPS->GetVelocity();
		const Engine::AtlasInfo &info = selectedPS->GetAtlasInfo();
		atlasColumns = info.nColumns;
		atlasRows = info.nRows;
		atlasCycles = info.cycles;
		psCenter = selectedPS->GetCenter();
		emissionBoxSize = selectedPS->GetEmissionBoxSize();
		emissionRadius = selectedPS->GetEmissionRadius();
		psColor = selectedPS->GetColor();
		fadeAlpha = selectedPS->GetFadeAlphaOverLifeTime();

		randomVelocity = selectedPS->UsesRandomVelocity();
		const glm::vec3 &low = selectedPS->GetRandomVelocityLow();
		if (glm::abs(low.x - low.y) > 0.001f)
		{
			velLowSeparate = low;
			velHighSeparate = selectedPS->GetRandomVelocityHigh();
			separateAxis = true;
		}
		else
		{
			velLow = low.x;
			velHigh = selectedPS->GetRandomVelocityHigh().x;
			separateAxis = false;
		}

		limitVelocity = selectedPS->GetLimitVelocityOverLifetime();
		speedLimit = selectedPS->GetSpeedLimit();
		dampen = selectedPS->GetDampen();

		gravityModifier = selectedPS->GetGravityModifier();

		useAtlas = selectedPS->UsesAtlas();
	}
	if (physicsManager->HasRigidBody(entity))
	{
		selectedRB = physicsManager->GetRigidBody(selectedEntity);

		visualizeRigidBody = selectedRB->WantsDebugView();
		kinematic = selectedRB->IsKinematic();
		mass = selectedRB->GetMass();
		restitution = selectedRB->GetRestitution();
		angularFactor = selectedRB->GetAngularFactor();
		linearFactor = selectedRB->GetLinearFactor();
		angSleepingThresh = selectedRB->GetHandle()->getAngularSleepingThreshold();
		linSleepingThresh = selectedRB->GetHandle()->getLinearSleepingThreshold();
		rigidBodyCenter = selectedRB->GetCenter();
		friction = selectedRB->GetHandle()->getFriction();

		if (selectedRB->GetShapeType() == BOX_SHAPE_PROXYTYPE)
		{
			rigidBodyShapeID = 0;
			rigidBodySize = selectedRB->GetBoxSize();
		}
		else if (selectedRB->GetShapeType() == SPHERE_SHAPE_PROXYTYPE)
		{
			rigidBodyShapeID = 1;
			rigidBodySphereRadius = selectedRB->GetRadius();
		}
		else if (selectedRB->GetShapeType() == CAPSULE_SHAPE_PROXYTYPE)
		{
			rigidBodyShapeID = 2;
			rigidBodyCapsuleHeight = selectedRB->GetHeight();
			rigidBodyCapsuleRadius = selectedRB->GetRadius();
		}
	}
	if (physicsManager->HasCollider(entity))
	{
		selectedCollider = physicsManager->GetCollider(entity);

		colliderCenter = selectedCollider->GetCenter();
		friction = selectedCollider->GetHandle()->getFriction();

		if (selectedCollider->GetShapeType() == BOX_SHAPE_PROXYTYPE)
		{
			colliderShapeID = 0;
			colliderBoxSize = selectedCollider->GetBoxSize();
		}
		else if (selectedCollider->GetShapeType() == SPHERE_SHAPE_PROXYTYPE)
		{
			colliderShapeID = 1;
			colliderRadius = selectedCollider->GetRadius();
		}
		else if (selectedCollider->GetShapeType() == CAPSULE_SHAPE_PROXYTYPE)
		{
			colliderShapeID = 2;
			colliderCapsuleRadius = selectedCollider->GetRadius();
			colliderCapsuleHeight = selectedCollider->GetHeight();
		}
	}
	if (physicsManager->HasTrigger(entity))
	{
		selectedTrigger = physicsManager->GetTrigger(entity);

		triggerCenter = selectedTrigger->GetCenter();

		if (selectedTrigger->GetShapeType() == BOX_SHAPE_PROXYTYPE)
		{
			shapeComboId = 0;
			triggerBoxSize = selectedTrigger->GetBoxSize();
		}
		else if (selectedTrigger->GetShapeType() == SPHERE_SHAPE_PROXYTYPE)
		{
			shapeComboId = 1;
			triggerRadius = selectedTrigger->GetRadius();
		}
		else if (selectedTrigger->GetShapeType() == CAPSULE_SHAPE_PROXYTYPE)
		{
			shapeComboId = 2;
			triggerCapsuleRadius = selectedTrigger->GetRadius();
			triggerCapsuleHeight = selectedTrigger->GetHeight();
		}
	}
	if (game->GetScriptManager().HasScript(entity))
	{
		selectedScript = game->GetScriptManager().GetScript(entity);
	}
	if (game->GetLightManager().HasPointLight(entity))
	{
		selectedLight = game->GetLightManager().GetPointLight(entity);
	}
	if (game->GetSoundManager().HasSoundSource(entity))
	{
		selectedSoundSource = game->GetSoundManager().GetSoundSource(entity);

		soundComboId = selectedSoundSource->IsStream() == true ? 1 : 0;
		volume = selectedSoundSource->GetVolume();
		pitch = selectedSoundSource->GetPitch();
		is3D = selectedSoundSource->Is3D();
		loopSound = selectedSoundSource->IsLooping();
		playOnStart = selectedSoundSource->IsPlayingOnStart();
		min3DDistance = selectedSoundSource->GetMin3DDistance();
		max3DDistance = selectedSoundSource->GetMax3DDistance();
	}
	if (game->GetUIManager().HasWidget(entity))
	{
		selectedWidget = game->GetUIManager().GetWidget(entity);

		isEnabled = selectedWidget->IsEnabled();
		positionPercent = selectedWidget->GetPosPercent();
		widgetSize = selectedWidget->GetRect().size;
		imageColor = selectedWidget->GetColorTint();
		depth = selectedWidget->GetDepth();

		if (selectedWidget->GetType() == Engine::WidgetType::BUTTON)
		{
			Engine::Button *b = static_cast<Engine::Button*>(selectedWidget);

			// Make sure the button's text is small enough to fit in the input text
			if (b->GetText().length() < 255)
				std::strcpy(text, b->GetText().c_str());

			textScale = b->GetTextScale();
			textColor = b->GetTextColor();
			textPosOffset = b->GetTextPosOffset();
		}
		else if (selectedWidget->GetType() == Engine::WidgetType::TEXT)
		{
			Engine::StaticText *t = static_cast<Engine::StaticText*>(selectedWidget);

			// Make sure the button's text is small enough to fit in the input text
			if (t->GetText().length() < 255)
				std::strcpy(text, t->GetText().c_str());

			textScale = t->GetTextScale();
		}
		else if (selectedWidget->GetType() == Engine::WidgetType::EDIT_TEXT)
		{
			Engine::EditText *et = static_cast<Engine::EditText*>(selectedWidget);

			// Make sure the button's text is small enough to fit in the input text
			if (et->GetText().length() < 255)
				std::strcpy(text, et->GetText().c_str());

			textScale = et->GetTextScale();
		}
	}
}

void ObjectWindow::HandleAI()
{
	/*if (ImGui::CollapsingHeader("AI", ImGuiTreeNodeFlags_DefaultOpen))
	{
		ImGui::Indent();

		std::string targetStr = "Target: " + targetName;
		ImGui::Text(targetStr.c_str());

		if (ImGui::TreeNode("Choose target from objects:"))
		{
			/*const std::vector<Engine::Object*> &objects = game->GetSceneManager()->GetObjects();
			const std::vector<std::string> names = editorManager->GetSceneWindow().GetNames();
			for (size_t i = 0; i < names.size(); i++)
			{
				// We can't target ourselves
				if (i == obj->GetID())
					continue;

				if (ImGui::Selectable(names[i].c_str()))
				{
					targetName = names[i];
					Engine::AIObject *aiObj = static_cast<Engine::AIObject*>(obj);
					aiObj->SetTarget(objects[i]);
					//target = objects[i];
				}
			}//
			ImGui::TreePop();
		}

		Engine::AIObject *aiObj = static_cast<Engine::AIObject*>(obj);		// No need for dynamic_cast because if we're in this function then the object is an AIObject

		if (ImGui::DragFloat3("Eyes offset", glm::value_ptr(eyesOffset), 0.1f))
			aiObj->SetEyesOffset(eyesOffset);
		if (ImGui::DragFloat("Eyes range", &eyesRange, 0.1f))
			aiObj->SetEyesRange(eyesRange);
		if (ImGui::DragFloat("Attack range", &attackRange, 0.1f))
			aiObj->SetAttackRange(attackRange);
		if (ImGui::DragFloat("Attack delay", &attackDelay, 0.1f))
			aiObj->SetAttackDelay(attackDelay);
		if (ImGui::DragFloat("Field of view", &fov, 0.1f, 0.0f))
		{
			if (fov <= 0.0f)
				fov = 0.1f;
			aiObj->SetFieldOfView(fov);
		}

		ImGui::Unindent();
	}*/
}

void ObjectWindow::HandleTransform()
{
	if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen))
	{
		bool transformChanged = false;
		bool isDragging = false;

		ImGui::Indent();
		if (ImGui::Button("Reset"))
		{
			position = glm::vec3(0.0f);
			rotation = glm::vec3(0.0f);
			scale = glm::vec3(1.0f);

			transformManager->SetLocalPosition(selectedEntity, position);
			transformManager->SetLocalRotation(selectedEntity, glm::quat(1.0f, 0.0f, 0.0f, 0.0f));
			transformManager->SetLocalScale(selectedEntity, scale);
			transformChanged = true;
		}
		if (ImGui::DragFloat3("Position", glm::value_ptr(position), 0.1f))
		{
			transformManager->SetLocalPosition(selectedEntity, position);
			transformChanged = true;
			if (Engine::Input::IsMouseButtonDown(0))
				isDragging = true;
		}
		if (ImGui::DragFloat3("Rotation", glm::value_ptr(rotation), 0.1f))
		{
			transformManager->SetLocalRotation(selectedEntity, glm::quat(glm::vec3(glm::radians(rotation.x), glm::radians(rotation.y), glm::radians(rotation.z))));
			//transformManager->SetLocalRotation(transformInstance, );
			//obj->SetLocalRotationEuler(rotation);
			transformChanged = true;
		}
		if (ImGui::DragFloat3("Scale", glm::value_ptr(scale), 0.1f))
		{
			transformManager->SetLocalScale(selectedEntity, scale);
			transformChanged = true;
		}

		ImGui::Unindent();

		if (transformChanged)
		{
			glm::mat4 localToWorld = transformManager->GetLocalToWorld(selectedEntity);
			// Normalize scale
			localToWorld[0] = glm::normalize(localToWorld[0]);
			localToWorld[1] = glm::normalize(localToWorld[1]);
			localToWorld[2] = glm::normalize(localToWorld[2]);

			if (selectedTrigger)
				selectedTrigger->SetTransform(localToWorld);

			if (selectedCollider)
				selectedCollider->SetTransform(localToWorld);

			if (selectedRB)
				selectedRB->SetPosition(localToWorld[3]);

			// Also update all child triggers, colliders, rbs


			//TransformCommand *tc = new TransformCommand(obj, position, rotation, scale);
			//tc->Execute();
			//editorManager->GetUndoStack().push(tc);
		}
	}
}

void ObjectWindow::HandleModel()
{
	if (!selectedModel)
		return;

	if (ImGui::CollapsingHeader("Model", &modelOpen, ImGuiTreeNodeFlags_DefaultOpen))
	{
		ImGui::Indent();

		const std::vector<Engine::MeshMaterial> &meshesAndMaterials = selectedModel->GetMeshesAndMaterials();

		ImGui::Text(selectedModel->GetPath().c_str());

		// If the model is missing then the meshes and materials will be 0
		if (meshesAndMaterials.size() == 0)
		{
			ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.25f, 1.0f), "Model is missing!");
			ImGui::Unindent();
			return;
		}

		if (ImGui::Checkbox("Cast shadows", &castShadows))
			selectedModel->SetCastShadows(castShadows);

		if (ImGui::DragFloat("LOD Distance", &lodDistance, 10.0f))
			selectedModel->SetLODDistance(lodDistance);

		for (size_t i = 0; i < meshesAndMaterials.size(); i++)
		{
			if (ImGui::TreeNodeEx(std::string("Mesh " + std::to_string(i)).c_str(), ImGuiTreeNodeFlags_DefaultOpen))
			{
				const Engine::MeshMaterial &mm = meshesAndMaterials[i];

				std::string matPath = "Material: ";

				// Display the name if we have otherwise display the path
				if (strlen(mm.mat->name) > 0)
				{
					matPath += mm.mat->name;
				}
				else
				{
					matPath += mm.mat->path;
				}			

				if (ImGui::Selectable(matPath.c_str()))
				{
					ImGui::OpenPopup("Choose material");
					std::string dir = editorManager->GetCurrentProjectDir() + "/*";
					files.clear();
					Engine::utils::FindFilesInDirectory(files, dir, ".mat");
					Engine::utils::FindFilesInDirectory(files, "Data/Materials/*", ".mat");
				}

				if (mm.mat->path.size() > 0 && ImGui::IsItemHovered())
					ImGui::SetTooltip(mm.mat->path.c_str());

				if (ImGui::BeginPopup("Choose material"))
				{
					if (files.size() > 0)
					{
						for (size_t j = 0; j < files.size(); j++)
						{
							if (ImGui::Selectable(files[j].c_str()))
							{
								Engine::MaterialInstance *mat = game->GetRenderer()->CreateMaterialInstance(game->GetScriptManager(), files[j], mm.mesh.vao->GetVertexInputDescs());
								strncpy(mat->name, mm.mat->name, 64);
								selectedModel->SetMeshMaterial((unsigned short)i, mat);
							}
						}
					}
					else
						ImGui::Text("No materials found on project folder.");

					ImGui::EndPopup();
				}

				if (ImGui::Button("Open in material editor"))
				{
					editorManager->GetMaterialWindow().SetCurrentMaterialInstance(mm.mat);
					editorManager->GetMaterialWindow().Focus();
				}

				ImGui::TreePop();
			}
		}

		if (selectedModel->GetType() == Engine::ModelType::ANIMATED)
		{
			Engine::AnimatedModel *am = static_cast<Engine::AnimatedModel*>(selectedModel);
			const std::vector<Engine::Animation*> &animations = am->GetAnimations();

			ImGui::Text("Animations:");
			for (size_t i = 0; i < animations.size(); i++)
			{
				if (ImGui::Selectable(animations[i]->name.c_str()))
				{
					am->PlayAnimation(i);
				}
			}

			if (am->GetAnimationController())
			{
				if (ImGui::Button("Remove animation controller"))
					am->RemoveAnimationController();
				if (ImGui::Button("Open controller on animation window"))
					editorManager->GetAnimationWindow().OpenAnimationController(am->GetAnimationController()->GetPath());
			}
			else
			{
				if (ImGui::Button("Add animation controller"))
				{
					ImGui::OpenPopup("Choose animation controller");
					files.clear();
					Engine::utils::FindFilesInDirectory(files, editorManager->GetCurrentProjectDir() + "/*", ".animcontroller");
				}
			}

			if (ImGui::Button("Add animation from file..."))
			{
				ImGui::OpenPopup("Choose file");
				std::string dir = editorManager->GetCurrentProjectDir() + "/*";
				files.clear();
				Engine::utils::FindFilesInDirectory(files, dir, ".fbx");
				Engine::utils::FindFilesInDirectory(files, dir, ".anim");
			}
			if (ImGui::BeginPopup("Choose file"))
			{
				if (files.size() > 0)
				{
					for (size_t j = 0; j < files.size(); j++)
					{
						if (ImGui::Selectable(files[j].c_str()))
						{
							std::string newAnimPath = Engine::utils::RemoveExtensionFromFilePath(files[j]) + "anim";

							if (Engine::utils::DirectoryExists(newAnimPath) == false)
								Engine::AssimpLoader::LoadSeparateAnimation(game->GetFileManager(), files[j], newAnimPath);
							
							am->AddAnimation(game->GetModelManager().LoadAnimation(newAnimPath));
						}
					}
				}
				else
					ImGui::Text("No files found on project folder.");

				ImGui::EndPopup();
			}

			if (ImGui::BeginPopup("Choose animation controller"))
			{
				if (files.size() > 0)
				{
					for (size_t i = 0; i < files.size(); i++)
					{
						if (ImGui::Selectable(files[i].c_str()))
							am->SetAnimationController(game->GetFileManager(), files[i], &game->GetModelManager());
					}
				}
				else
					ImGui::Text("No files found on project folder.");

				ImGui::EndPopup();
			}

			ImGui::Text("Bone attachments: ");
			const Engine::BoneAttachment *boneAttachments = am->GetBoneAttachments();

			const Engine::BoneAttachment *curAttachment = boneAttachments;
			for (unsigned short i = 0; i < am->GetBoneAttachmentsCount(); i++)
			{
				ImGui::Text(curAttachment->bone->name.c_str());
				ImGui::SameLine();
				ImGui::PushID(i);
				if (ImGui::Button("Select attached object"))
				{
					//editorManager->GetGizmo().SetSelectedObject(curAttachment->obj);
					//editorManager->GetObjectWindow().SetObject(curAttachment->obj);
				}
				ImGui::PopID();
				ImGui::SameLine();

				if (ImGui::Button("Remove"))
				{
					am->RemoveBoneAttachment(game->GetEntityManager(), curAttachment);
				}
				curAttachment++;
			}
		}

		ImGui::Unindent();
	}
}

void ObjectWindow::HandleRigidBody()
{
	if (!selectedRB)
		return;

	if (ImGui::CollapsingHeader("Rigid Body", &rigidBodyOpen, ImGuiTreeNodeFlags_DefaultOpen))
	{
		ImGui::Indent();

		if (ImGui::Checkbox("Visualize shape", &visualizeRigidBody))
		{
			selectedRB->DebugView(visualizeRigidBody);
		}
		if (ImGui::DragFloat3("Center", glm::value_ptr(rigidBodyCenter), 0.1f))
		{
			selectedRB->SetCenter(rigidBodyCenter);
		}
		if (ImGui::Checkbox("Kinematic", &kinematic))
		{
			selectedRB->SetKinematic(kinematic);
		}
		if (ImGui::InputFloat("Mass", &mass, 0.05f))
		{
			selectedRB->SetMass(mass);
		}
		if (ImGui::InputFloat("Restitution", &restitution, 0.05f))
		{
			selectedRB->SetRestitution(restitution);
		}
		if (ImGui::DragFloat3("Angular Factor", glm::value_ptr(angularFactor), 0.05f))
		{
			selectedRB->SetAngularFactor(angularFactor);
		}
		if (ImGui::DragFloat3("Linear factor", &linearFactor.x, 0.05f))
		{
			selectedRB->SetLinearFactor(linearFactor.x, linearFactor.y, linearFactor.z);
		}
		if (ImGui::InputFloat("Angular Sleeping Threshold", &angSleepingThresh, 0.05f))
		{
			selectedRB->GetHandle()->setSleepingThresholds(linSleepingThresh, angSleepingThresh);
		}
		if (ImGui::InputFloat("Linear Sleeping Threshold", &linSleepingThresh, 0.05f))
		{
			selectedRB->GetHandle()->setSleepingThresholds(linSleepingThresh, angSleepingThresh);
		}
		if (ImGui::DragFloat("Friction", &friction, 0.01f))
		{
			if (friction < 0.0f)
				friction = 0.0f;
			selectedRB->GetHandle()->setFriction(friction);
		}
		

		// Shape dimensions
		if (rigidBodyShapeID == 0)
		{
			if (ImGui::DragFloat3("Size", glm::value_ptr(rigidBodySize), 0.05f))
			{
				selectedRB->SetBoxSize(rigidBodySize);
			}
		}
		else if (rigidBodyShapeID == 1)
		{
			if (ImGui::DragFloat("Radius", &rigidBodySphereRadius, 0.05f))
			{
				selectedRB->SetRadius(rigidBodySphereRadius);
			}
		}
		else if (rigidBodyShapeID == 2)
		{
			if (ImGui::DragFloat("Radius", &rigidBodyCapsuleRadius, 0.05f))
			{
				selectedRB->SetRadius(rigidBodyCapsuleRadius);
			}
			if (ImGui::DragFloat("Height", &rigidBodyCapsuleHeight, 0.05f))
			{
				selectedRB->SetCapsuleHeight(rigidBodyCapsuleHeight);
			}
		}

		ImGui::Unindent();
	}
}

void ObjectWindow::HandleParticleSystem()
{
	if (!selectedPS)
		return;

	if (ImGui::CollapsingHeader("Particle System", &particleSystemOpen, ImGuiTreeNodeFlags_DefaultOpen))
	{
		ImGui::Indent();

		matNameString = "Material: " + selectedPS->GetMaterialInstance()->path;

		if (ImGui::Button(matNameString.c_str()))
		{
			files.clear();
			Engine::utils::FindFilesInDirectory(files, editorManager->GetCurrentProjectDir() + "/*", ".mat");
			ImGui::OpenPopup("Choose material");
		}

		if (ImGui::BeginPopup("Choose material"))
		{
			for (size_t i = 0; i < files.size(); i++)
			{
				size_t lastBar = files[i].find_last_of('/');

				if (ImGui::Selectable(files[i].substr(lastBar + 1).c_str()))		// +1 so we don't include the bar
				{
					selectedPS->SetMaterialInstance(game->GetRenderer()->CreateMaterialInstance(game->GetScriptManager(), files[i], selectedPS->GetMesh().vao->GetVertexInputDescs()));
				}
			}
			ImGui::EndPopup();
		}

		if (ImGui::Button("Play"))
			selectedPS->Play();

		ImGui::SameLine();

		if (ImGui::Button("Stop"))
			selectedPS->Stop();

		if (ImGui::Checkbox("Loop", &loopPs))
			selectedPS->SetIsLooping(loopPs);

		ImGui::PushID(12345678);
		if (ImGui::DragFloat3("Center", glm::value_ptr(psCenter), 0.1f))
			selectedPS->SetCenter(psCenter);
		ImGui::PopID();

		if (ImGui::InputFloat("Duration", &duration, 0.05f, 0.0f, "%.2f"))
		{
			if (duration < 0.05f)
				duration = 0.05f;

			selectedPS->SetDuration(duration);
		}

		if (ImGui::InputFloat("Lifetime", &psLifetime, 0.05f, 0.0f, "%.2f"))
		{
			if (psLifetime < 0.05f)
				psLifetime = 0.05f;

			selectedPS->SetLifeTime(psLifetime);
		}

		if (ImGui::InputFloat("Size", &size, 0.01f, 0.0f, "%.2f"))
		{
			if (size < 0.01f)
				size = 0.01f;

			selectedPS->SetSize(size);
		}

		if (ImGui::TreeNode("Emission"))
		{
			if (ImGui::InputInt("Emission", &emission))
			{
				if (emission < 0)
					emission = 0;

				selectedPS->SetEmission(emission);
			}

			ImGui::Combo("Emission Shape", &emissionShapeID, "Box\0Sphere");

			if (emissionShapeID == 0)
			{
				if (ImGui::DragFloat3("Box Size", glm::value_ptr(emissionBoxSize), 0.1f))
				{
					selectedPS->SetEmissionBox(emissionBoxSize);
				}
			}
			else if (emissionShapeID == 1)
			{
				if (ImGui::DragFloat("Radius", &emissionRadius, 0.1f, 0.1f))
				{
					selectedPS->SetEmissionRadius(emissionRadius);
				}
			}
			ImGui::TreePop();
		}

		if (ImGui::InputInt("Max Particles", &maxParticles))
		{
			selectedPS->SetMaxParticles(maxParticles);
		}

		if (ImGui::Checkbox("Random velocity", &randomVelocity))
		{
			if (randomVelocity)
			{
				velLowSeparate = selectedPS->GetRandomVelocityLow();
				velHighSeparate = selectedPS->GetRandomVelocityHigh();
				selectedPS->SetRandomVelocity(velLowSeparate, velHighSeparate);
			}
			else
			{
				selectedPS->SetVelocity(velocity);
			}
		}

		if (randomVelocity)
		{
			ImGui::Checkbox("Separate axis", &separateAxis);
			if (separateAxis)
			{
				if (ImGui::DragFloat3("Velocity low", &velLowSeparate.x, 0.1f))
					selectedPS->SetRandomVelocity(velLowSeparate, velHighSeparate);
				if (ImGui::DragFloat3("Velocity high", &velHighSeparate.x, 0.1f))
					selectedPS->SetRandomVelocity(velLowSeparate, velHighSeparate);
			}
			else
			{
				if (ImGui::DragFloat("Velocity low", &velLow, 0.1f))
					selectedPS->SetRandomVelocity(glm::vec3(velLow), glm::vec3(velHigh));
				if (ImGui::DragFloat("Velocity high", &velHigh, 0.1f))
					selectedPS->SetRandomVelocity(glm::vec3(velLow), glm::vec3(velHigh));
			}
		}
		else
		{
			if (ImGui::DragFloat3("Velocity", glm::value_ptr(velocity), 0.1f))
			{
				selectedPS->SetVelocity(velocity);
			}
		}

		if (ImGui::Checkbox("Limit velocity over life time", &limitVelocity))
			selectedPS->SetLimitVelocityOverLifeTime(limitVelocity);

		if (limitVelocity)
		{
			if (ImGui::DragFloat("Speed limit", &speedLimit, 0.05f))
				selectedPS->SetSpeedLimit(speedLimit);
			if (ImGui::DragFloat("Dampen", &dampen, 0.05f))
				selectedPS->SetDampen(dampen);
		}

		if (ImGui::DragFloat("Gravity modifier", &gravityModifier, 0.05f))
			selectedPS->SetGravityModifier(gravityModifier);
		if (ImGui::ColorEdit4("Color tint:", &psColor.x))
			selectedPS->SetColor(psColor);
		if (ImGui::Checkbox("Fade alpha over lifetime", &fadeAlpha))
			selectedPS->SetFadeAlphaOverLifetime(fadeAlpha);
		if (ImGui::Checkbox("Use atlas", &useAtlas))
			selectedPS->SetUseAtlas(useAtlas);

		if (useAtlas)
		{
			if (ImGui::DragInt("Columns", &atlasColumns, 1.0f, 1))
			{
				if (atlasColumns <= 0)
					atlasColumns = 1;
				selectedPS->SetAtlasInfo(atlasColumns, atlasRows, atlasCycles);
			}
			if (ImGui::DragInt("Rows", &atlasRows, 1.0f, 1))
			{
				if (atlasRows <= 0)
					atlasRows = 1;
				selectedPS->SetAtlasInfo(atlasColumns, atlasRows, atlasCycles);
			}
			if (ImGui::DragFloat("Cycles", &atlasCycles, 0.1f, 0.05f))
			{
				if (atlasCycles <= 0.0f)
					atlasCycles = 0.1f;
				selectedPS->SetAtlasInfo(atlasColumns, atlasRows, atlasCycles);
			}
		}

		ImGui::Unindent();
	}
}

void ObjectWindow::HandleTrigger()
{
	if (!selectedTrigger)
		return;

	if (ImGui::CollapsingHeader("Trigger", &triggerOpen, ImGuiTreeNodeFlags_DefaultOpen))
	{
		ImGui::Indent();

		if (ImGui::Checkbox("Visualize", &visualize))
			selectedTrigger->DebugView(visualize);

		if (selectedTrigger->HasCollisionEnabled())
		{
			if (ImGui::Button("Disable collision"))
				selectedTrigger->DisableCollision();
		}
		else
		{
			if (ImGui::Button("Enable collision"))
				selectedTrigger->EnableCollision();
		}
		if (ImGui::IsItemHovered())
			ImGui::SetTooltip("Enable collision for this trigger. OnTriggerEnter will function like OnCollisionEnter");

		if (ImGui::DragFloat3("Center", glm::value_ptr(triggerCenter), 0.1f))
		{
			selectedTrigger->SetCenter(transformManager->GetLocalToWorld(selectedEntity), triggerCenter);
		}
		if (shapeComboId == 0)
		{
			if (ImGui::DragFloat3("Size", glm::value_ptr(triggerBoxSize), 0.05f))
				selectedTrigger->SetBoxSize(triggerBoxSize);
		}
		else if (shapeComboId == 1)
		{
			if (ImGui::DragFloat("Radius", &triggerRadius, 0.05f))
				selectedTrigger->SetRadius(triggerRadius);
		}
		else if (shapeComboId == 2)
		{
			if (ImGui::DragFloat("Radius", &triggerCapsuleRadius, 0.05f))
				selectedTrigger->SetRadius(triggerCapsuleRadius);

			if (ImGui::DragFloat("Height", &triggerCapsuleHeight, 0.05f))
				selectedTrigger->SetCapsuleHeight(triggerCapsuleHeight);
		}
		ImGui::Unindent();
	}
}

void ObjectWindow::HandleScript()
{
	if (!selectedScript)
		return;

	if (ImGui::CollapsingHeader("Script", &scriptOpen, ImGuiTreeNodeFlags_DefaultOpen))
	{
		ImGui::Indent();

		std::string name = selectedScript->GetPath();
		size_t lastBar = name.find_last_of('/');
		name.erase(0, lastBar + 1);		// +1 to remove the /

		ImGui::Text(name.c_str());

		const std::vector<Engine::ScriptProperty> &properties = selectedScript->GetProperties();

		for (size_t i = 0; i < properties.size(); i++)
		{
			if (ImGui::Selectable(properties[i].name.c_str()))
			{
				ImGui::OpenPopup("Choose:");
				propertyIndex = i;
			}
			ImGui::SameLine();
			ImGui::Text(": ");

			if (properties[i].e.IsValid())
			{
				ImGui::SameLine();
				ImGui::Text(editorManager->GetEditorNameManager().GetName(properties[i].e));
			}

			/*if (ImGui::BeginDragDropTarget())
			{
				if (ImGui::AcceptDragDropPayload("object_id"))
				{
				}
				ImGui::EndDragDropTarget();
			}*/

		}
		if (ImGui::BeginPopup("Choose:"))
		{
			const std::vector<EditorName> &editorNames = editorManager->GetEditorNameManager().GetNames();
			std::vector<std::string> names(editorNames.size());
			for (size_t i = 0; i < names.size(); i++)
			{
				names[i] = editorNames[i].name;
			}

			if (ImGui::ListBox("Choose object:", &selectedScriptProperty, names))
			{
				selectedScript->SetProperty(properties[propertyIndex].name, editorNames[selectedScriptProperty].e);
				ImGui::CloseCurrentPopup();
			}
			ImGui::EndPopup();
		}

		ImGui::Unindent();
	}
}

void ObjectWindow::HandleCollider()
{
	if (!selectedCollider)
		return;

	if (ImGui::CollapsingHeader("Collider", &colliderOpen, ImGuiTreeNodeFlags_DefaultOpen))
	{
		ImGui::Indent();

		if (ImGui::Checkbox("Visualize", &visualize))
			selectedCollider->DebugView(visualize);

		if (ImGui::DragFloat3("Center", glm::value_ptr(colliderCenter), 0.1f))
			selectedCollider->SetCenter(colliderCenter);

		if (colliderShapeID == 0)
		{
			if (ImGui::DragFloat3("Size", glm::value_ptr(colliderBoxSize), 0.05f))
				selectedCollider->SetBoxSize(colliderBoxSize);
		}
		else if (colliderShapeID == 1)
		{
			if (ImGui::DragFloat("Radius", &colliderRadius, 0.05f))
				selectedCollider->SetRadius(colliderRadius);
		}
		else if (colliderShapeID == 2)
		{
			if (ImGui::DragFloat("Radius", &colliderCapsuleRadius, 0.05f))
				selectedCollider->SetRadius(colliderCapsuleRadius);
			if (ImGui::DragFloat("Height", &colliderCapsuleHeight, 0.05f))
				selectedCollider->SetCapsuleHeight(colliderCapsuleHeight);
		}

		if (ImGui::DragFloat("Friction", &friction, 0.01f))
		{
			if (friction < 0.0f)
				friction = 0.0f;
			selectedCollider->GetHandle()->setFriction(friction);
		}

		ImGui::Unindent();
	}
}

void ObjectWindow::HandleLight()
{
	if (!selectedLight)
		return;

	if (ImGui::CollapsingHeader("Light", &lightOpen, ImGuiTreeNodeFlags_DefaultOpen))
	{
		ImGui::Indent();

		bool changed = false;

		/*if (*/ImGui::Checkbox("Cast shadows", &selectedLight->castShadows);//)
			//changed = true;

		/*if (light->type == LightType::DIRECTIONAL)
		{
			ImGui::Text("Light type: Directional");

		}
		else */if (selectedLight->type == Engine::LightType::SPOT)
		{
			ImGui::Text("Light type: Spot");
		}
		else if (selectedLight->type == Engine::LightType::POINT)
		{
			ImGui::Text("Light type: Point");

			Engine::PointLight *pl = static_cast<Engine::PointLight*>(selectedLight);

			if (ImGui::DragFloat3("Offset", &pl->positionOffset.x, 0.01f, 0.0f))
				changed = true;

			if (ImGui::DragFloat("Radius", &pl->radius, 0.05f, 0.0f))
				changed = true;

			if (ImGui::DragFloat("Intensity", &pl->intensity, 0.05f, 0.0f))
				changed = true;

			if (ImGui::ColorEdit3("Color", &pl->color.x))
				changed = true;

			if (changed)
				game->GetLightManager().UpdatePointLights();
		}

		ImGui::Unindent();
	}
}

void ObjectWindow::HandleSoundSource()
{
	if (!selectedSoundSource)
		return;

	if (ImGui::CollapsingHeader("Sound Source", &soundSourceOpen, ImGuiTreeNodeFlags_DefaultOpen))
	{
		ImGui::Indent();

		ImGui::Text(selectedSoundSource->GetPath().c_str());

		ImGui::Combo("Sound type", &soundComboId, "Sound effect\0Music");

		bool buttonClicked = false;

		/*if (selectedSoundSource->GetSoundHandle() == nullptr)
		{
			if (soundComboId == 0)
			{
				if (ImGui::Button("Add Sound effect"))
					buttonClicked = true;
			}
			else if (soundComboId == 1)
			{
				if (ImGui::Button("Add Music"))
					buttonClicked = true;
			}
		}
		else
		{
			if (soundComboId == 0)
			{
				if (ImGui::Button("Replace Sound effect"))
					buttonClicked = true;
			}
			else if (soundComboId == 1)
			{
				if (ImGui::Button("Replace Music"))
					buttonClicked = true;
			}
		}*/

		if (buttonClicked)
		{
			files.clear();
			Engine::utils::FindFilesInDirectory(files, editorManager->GetCurrentProjectDir() + "/*", ".wav");
			Engine::utils::FindFilesInDirectory(files, editorManager->GetCurrentProjectDir() + "/*", ".ogg");
			ImGui::OpenPopup("Choose sound file");
		}

		if (ImGui::BeginPopup("Choose sound file"))
		{
			for (size_t i = 0; i < files.size(); i++)
			{
				size_t lastBar = files[i].find_last_of('/');

				if (ImGui::Selectable(files[i].substr(lastBar + 1).c_str()))		// +1 so we don't include the bar
				{
					if (soundComboId == 0)
						game->GetSoundManager().LoadSound(selectedSoundSource, files[i], false);
					else if (soundComboId == 1)
						game->GetSoundManager().LoadSound(selectedSoundSource, files[i], true);
				}
			}
			ImGui::EndPopup();
		}

		if (ImGui::Button("Play sound"))
			selectedSoundSource->Play();

		ImGui::SameLine();

		if (ImGui::Button("Stop sound"))
			selectedSoundSource->Stop();

		if (ImGui::Checkbox("Loop sound", &loopSound))
			selectedSoundSource->EnableLoop(loopSound);

		if (ImGui::Checkbox("3D", &is3D))
			selectedSoundSource->Enable3D(is3D);

		if (ImGui::Checkbox("Play on start", &playOnStart))
			selectedSoundSource->EnablePlayOnStart(playOnStart);

		if (ImGui::DragFloat("Min 3D distance", &min3DDistance, 0.1f))
			selectedSoundSource->SetMin3DDistance(min3DDistance);

		if (ImGui::DragFloat("Max 3D distance", &max3DDistance, 0.1f))
			selectedSoundSource->SetMax3DDistance(max3DDistance);

		if (ImGui::DragFloat("Volume", &volume, 0.05f, 0.0f))
			selectedSoundSource->SetVolume(volume);

		if (ImGui::DragFloat("Pitch", &pitch, 0.05f, 0.0f))
			selectedSoundSource->SetPitch(pitch);

		ImGui::Unindent();
	}
}

void ObjectWindow::HandleWidget()
{
	if (!selectedWidget)
		return;

	if (ImGui::CollapsingHeader("Widget", &widgetOpen, ImGuiTreeNodeFlags_DefaultOpen))
	{
		ImGui::Indent();

		if (textScaleChanged)
		{
			widgetSize = selectedWidget->GetRect().size;
			textScaleChanged = false;
		}

		switch (selectedWidget->GetType())
		{
		case Engine::WidgetType::TEXT:
			ImGui::Text("Type: Text");
			break;
		case Engine::WidgetType::BUTTON:
			ImGui::Text("Type: Button");
			break;
		case Engine::WidgetType::IMAGE:
			ImGui::Text("Type: Image");
			break;
		case Engine::WidgetType::EDIT_TEXT:
			ImGui::Text("Type: Edit text");
			break;
		}

		if (ImGui::Checkbox("Enabled", &isEnabled))
			selectedWidget->SetEnabled(isEnabled);

		if (ImGui::DragFloat("Depth", &depth, 0.01f))
			selectedWidget->SetDepth(depth);

		if (ImGui::DragFloat2("Position (Percent)", glm::value_ptr(positionPercent), 1.0f))
			selectedWidget->SetRectPosPercent(positionPercent);

		if (ImGui::DragFloat2("Size", glm::value_ptr(widgetSize), 1.0f))
		{
			if (widgetSize.x <= 0.1f)
				widgetSize.x = 10.0f;
			if (widgetSize.y <= 0.1f)
				widgetSize.y = 10.0f;

			selectedWidget->SetRectSize(widgetSize);
		}

		if (ImGui::ColorEdit4("Color", &imageColor.x))
			selectedWidget->SetColorTintRGBA(imageColor);

		if (selectedWidget->GetType() == Engine::WidgetType::TEXT)
		{
			HandleText();
		}
		else if (selectedWidget->GetType() == Engine::WidgetType::BUTTON)
		{
			HandleButton();
			HandleTextures();
		}
		else if (selectedWidget->GetType() == Engine::WidgetType::IMAGE)
		{
			HandleImage();
		}
		else if (selectedWidget->GetType() == Engine::WidgetType::EDIT_TEXT)
		{
			HandleEditText();
		}


		ImGui::Unindent();
	}
}

void ObjectWindow::HandleButton()
{
	Engine::Button *button = static_cast<Engine::Button*>(selectedWidget);

	if (ImGui::InputText("Text", text, 256))
	{
		button->SetText(std::string(text));
	}
	if (ImGui::DragFloat2("Text scale", glm::value_ptr(textScale), 0.05f, 0.0f))
	{
		if (textScale.x <= 0.1f)
			textScale.x = 1.0f;
		if (textScale.y <= 0.1f)
			textScale.y = 1.0f;

		button->SetTextScale(textScale);
	}
	if (ImGui::DragFloat2("Text position offset", &textPosOffset.x, 0.05f, 0.0f))
	{
		button->SetTextPosOffset(textPosOffset);
	}
	if (ImGui::ColorEdit4("Text Color", &textColor.x))
	{
		button->SetTextColor(textColor);
	}
	if (ImGui::Button("Fit button to text"))
	{
		button->SetText(std::string(text));
		textScaleChanged = true;
	}
}

void ObjectWindow::HandleText()
{
	Engine::StaticText *t = static_cast<Engine::StaticText*>(selectedWidget);

	if (ImGui::InputText("Text", text, 255))
	{
		t->SetText(std::string(text));
	}
	if (ImGui::DragFloat2("Text scale", glm::value_ptr(textScale), 0.05f, 0.0f))
	{
		if (textScale.x <= 0.1f)
			textScale.x = 1.0f;
		if (textScale.y <= 0.1f)
			textScale.y = 1.0f;

		t->SetTextScale(textScale);
	}
	if (ImGui::Button("Fit to text"))
	{
		t->SetText(std::string(text));
		textScaleChanged = true;
	}
}

void ObjectWindow::HandleTextures()
{
	if (ImGui::Selectable("Idle Texture"))
	{
		choosenTextureType = 0;
		addTextureSelected = true;

		ImGui::OpenPopup("Choose texture");
		std::string dir = editorManager->GetCurrentProjectDir() + "/*";
		files.clear();
		Engine::utils::FindFilesInDirectory(files, dir, ".dds");
		Engine::utils::FindFilesInDirectory(files, dir, ".png");
	}
	if (ImGui::Selectable("Hover Texture"))
	{
		choosenTextureType = 1;
		addTextureSelected = true;

		ImGui::OpenPopup("Choose texture");
		std::string dir = editorManager->GetCurrentProjectDir() + "/*";
		files.clear();
		Engine::utils::FindFilesInDirectory(files, dir, ".dds");
		Engine::utils::FindFilesInDirectory(files, dir, ".png");
	}

	if (ImGui::Selectable("Pressed Texture"))
	{
		choosenTextureType = 2;
		addTextureSelected = true;

		ImGui::OpenPopup("Choose texture");
		std::string dir = editorManager->GetCurrentProjectDir() + "/*";
		files.clear();
		Engine::utils::FindFilesInDirectory(files, dir, ".dds");
		Engine::utils::FindFilesInDirectory(files, dir, ".png");
	}

	if (addTextureSelected)
	{
		if (ImGui::BeginPopup("Choose texture"))
		{
			if (files.size() > 0)
				AddTexture();
			else
				ImGui::Text("No textures found on project folder.");

			ImGui::EndPopup();
		}
		else
			addTextureSelected = false;
	}
}

void ObjectWindow::HandleImage()
{
	Engine::Image *image = static_cast<Engine::Image*>(selectedWidget);

	if (ImGui::Selectable("Texture"))
	{
		addTextureSelected = true;

		ImGui::OpenPopup("Choose texture");
		std::string dir = editorManager->GetCurrentProjectDir() + "/*";
		files.clear();
		Engine::utils::FindFilesInDirectory(files, dir, ".dds");
		Engine::utils::FindFilesInDirectory(files, dir, ".png");
	}

	if (addTextureSelected)
	{
		if (ImGui::BeginPopup("Choose texture"))
		{
			if (files.size() > 0)
			{
				for (size_t i = 0; i < files.size(); i++)
				{
					if (ImGui::Selectable(files[i].c_str()))
					{
						Engine::TextureParams params = { Engine::TextureWrap::REPEAT, Engine::TextureFilter::LINEAR, Engine::TextureFormat::RGBA, Engine::TextureInternalFormat::SRGB8_ALPHA8, Engine::TextureDataType::UNSIGNED_BYTE, false };
						image->SetTexture(game->GetRenderer()->CreateTexture2D(files[i], params));
					}
				}
			}
			else
				ImGui::Text("No textures found on project folder.");

			ImGui::EndPopup();
		}
		else
			addTextureSelected = false;
	}
}

void ObjectWindow::HandleEditText()
{
	Engine::EditText *editText = static_cast<Engine::EditText*>(selectedWidget);

	if (ImGui::InputText("Text", text, 255))
		editText->SetText(std::string(text));

	if (ImGui::DragFloat2("Text scale", glm::value_ptr(textScale), 0.05f, 0.0f))
	{
		if (textScale.x <= 0.1f)
			textScale.x = 1.0f;
		if (textScale.y <= 0.1f)
			textScale.y = 1.0f;

		editText->SetTextScale(textScale);
	}

	if (ImGui::Button("Fit rect to text"))
		editText->SetRectSize(game->GetRenderingPath()->GetFont().CalculateTextSize(editText->GetText(), editText->GetTextScale()));

	if (ImGui::Selectable("Background texture"))
	{
		addTextureSelected = true;

		ImGui::OpenPopup("Choose texture");
		std::string dir = editorManager->GetCurrentProjectDir() + "/*";
		files.clear();
		Engine::utils::FindFilesInDirectory(files, dir, ".dds");
		Engine::utils::FindFilesInDirectory(files, dir, ".png");
	}

	if (addTextureSelected)
	{
		if (ImGui::BeginPopup("Choose texture"))
		{
			if (files.size() > 0)
			{
				for (size_t i = 0; i < files.size(); i++)
				{
					if (ImGui::Selectable(files[i].c_str()))
						editText->SetBackgroundTexture(game->GetRenderer()->CreateTexture2D(files[i], { Engine::TextureWrap::REPEAT, Engine::TextureFilter::LINEAR, Engine::TextureFormat::RGBA, Engine::TextureInternalFormat::SRGB8_ALPHA8, Engine::TextureDataType::UNSIGNED_BYTE, false }));
				}
			}
			else
				ImGui::Text("No textures found on project folder.");

			ImGui::EndPopup();
		}
		else
			addTextureSelected = false;
	}
}

bool ObjectWindow::AddModel(bool animated)
{
	static ImGuiTextFilter filter;
	/*ImGui::Text("Filter usage:\n"
		"  \"\"         display all lines\n"
		"  \"xxx\"      display lines containing \"xxx\"\n"
		"  \"xxx,yyy\"  display lines containing \"xxx\" or \"yyy\"\n"
		"  \"-xxx\"     hide lines containing \"xxx\"");*/
	filter.Draw("Find");
	for (size_t i = 0; i < files.size(); i++)
		if (filter.PassFilter(files[i].c_str()))
		{
			if (ImGui::Selectable(files[i].c_str()))
			{
				// Load the model with assimp and save it in our custom format
				// Then call AddModel with the path to the custom model to load it and add it to the entity

				std::string newPath = Engine::utils::RemoveExtensionFromFilePath(files[i]) + "model";

				// Check if the model in the custom format exists, if not loaded the obj, fbx, etc with Assimp and save it in the custom format
				if (Engine::utils::DirectoryExists(newPath) == false)
				{
					if (animated)
						Engine::AssimpLoader::LoadAnimatedModel(game, files[i], {});
					else
						Engine::AssimpLoader::LoadModel(game, files[i], {});
				}

				modelManager->AddModel(selectedEntity, newPath, animated);
				modelOpen = true;
				isSelectingAsset = false;

				SetEntity(selectedEntity);		// Call SetEntity to update the model values

				return true;
			}
		}

	return false;
}

void ObjectWindow::AddRigidBody(Engine::ShapeType type)
{
	float mass = 1.0f;
	Engine::Layer layer = Engine::Layer::DEFAULT;

	//this->layer=layer;

	/*glm::vec3 center;
	center.x = (aabb.max.x + aabb.min.x) / 2.0f;
	center.y = (aabb.max.y + aabb.min.y) / 2.0f;
	center.z = (aabb.max.z + aabb.min.z) / 2.0f;

	btVector3 halfExtents = btVector3(
		aabb.max.x - center.x,
		aabb.max.y - center.y,
		aabb.max.z - center.z);*/
	btVector3 halfExtents = btVector3(0.5f, 0.5f, 0.5f);

	if (type == Engine::ShapeType::BOX)
	{
		selectedRB = physicsManager->AddBoxRigidBody(selectedEntity, halfExtents, mass, layer);
	}
	else if (type == Engine::ShapeType::SPHERE)
	{
		float radius = std::max(halfExtents.x(), std::max(halfExtents.y(), halfExtents.z()));

		if (radius > 0.0f)
			selectedRB = physicsManager->AddSphereRigidBody(selectedEntity, radius, mass, layer);
		else
			selectedRB = physicsManager->AddSphereRigidBody(selectedEntity, 0.5f, mass, layer);
	}
	else if (type == Engine::ShapeType::CAPSULE)
	{
		float radius = std::max(halfExtents.x(), std::max(halfExtents.y(), halfExtents.z()));
		//float height = aabb.max.y - aabb.min.y;
		float height = 1.0f;

		if (radius > 0.0f && height > 0.0f)
			selectedRB = physicsManager->AddCapsuleRigidBody(selectedEntity, radius, height, mass, layer);
		else
			selectedRB = physicsManager->AddCapsuleRigidBody(selectedEntity, 0.5f, 1.0f, mass, layer);
	}

	if (selectedRB)
	{
		const glm::mat4 &m = transformManager->GetLocalToWorld(selectedEntity);
		//glm::vec3 offsetFromOrigin = center - glm::vec3(m[3]);

		selectedRB->SetTransform(m);
		//rigidBody->SetCenter(offsetFromOrigin);					// Set center to compensate when the object's origin is not at it's center
	}

	SetEntity(selectedEntity);
	rigidBodyOpen = true;

	editorManager->GetGizmo().SetSelectedEntity(selectedEntity);		// So the gizmo updates the selected entity collider, rigidbody or trigger
}

void ObjectWindow::AddParticleSystem()
{
	selectedPS = game->GetParticleManager().AddParticleSystem(selectedEntity);
	SetEntity(selectedEntity);
	particleSystemOpen = true;
}

void ObjectWindow::AddTrigger(Engine::ShapeType type)
{
	glm::mat4 localToWorld = transformManager->GetLocalToWorld(selectedEntity);
	
	glm::vec3 pos = localToWorld[3];
	btVector3 center = btVector3(pos.x, pos.y, pos.z);

	glm::vec3 worldScale = glm::vec3(glm::length(localToWorld[0]), glm::length(localToWorld[1]), glm::length(localToWorld[2]));
	//glm::vec3 size = glm::vec3(1.0f);

	if (worldScale.x <= 0.01f || worldScale.y <= 0.01f || worldScale.z <= 0.01f)
		worldScale = glm::vec3(0.5f);

	if (type == Engine::ShapeType::BOX)
	{
		selectedTrigger = physicsManager->AddBoxTrigger(selectedEntity, center, btVector3(0.5f, 0.5f, 0.5f));
		selectedTrigger->SetBoxSize(worldScale);
	}
	else if (type == Engine::ShapeType::SPHERE)
	{
		float radius = glm::max(worldScale.x, glm::max(worldScale.y, worldScale.z));

		selectedTrigger = physicsManager->AddSphereTrigger(selectedEntity, center, 1.0f);
		selectedTrigger->SetRadius(radius);
	}
	else if (type == Engine::ShapeType::CAPSULE)
	{
		float radius = glm::max(worldScale.x, glm::max(worldScale.y, worldScale.z));

		selectedTrigger = physicsManager->AddCapsuleTrigger(selectedEntity, center, 0.5f, 1.0f);
		selectedTrigger->SetRadius(radius);
		selectedTrigger->SetCapsuleHeight(worldScale.y);
	}

	if (selectedTrigger)
	{
		//selectedTrigger->SetPosition(localToWorld[3]);

		// Normalize scale
		localToWorld[0] = glm::normalize(localToWorld[0]);
		localToWorld[1] = glm::normalize(localToWorld[1]);
		localToWorld[2] = glm::normalize(localToWorld[2]);

		selectedTrigger->SetTransform(localToWorld);

		if (selectedScript)
			selectedTrigger->SetScript(selectedScript);
	}

	SetEntity(selectedEntity);
	triggerOpen = true;

	editorManager->GetGizmo().SetSelectedEntity(selectedEntity);		// So the gizmo updates the selected entity collider, rigidbody or trigger
}

void ObjectWindow::AddCollider(Engine::ShapeType type)
{
	const glm::mat4 &localToWorld = transformManager->GetLocalToWorld(selectedEntity);

	glm::vec3 worldPos = localToWorld[3];
	btVector3 center = btVector3(worldPos.x, worldPos.y, worldPos.z);

	glm::vec3 worldScale = glm::vec3(glm::length(localToWorld[0]), glm::length(localToWorld[1]), glm::length(localToWorld[2]));
	//glm::vec3 size = glm::vec3(1.0f);

	/*btVector3 halfExtents = btVector3(
		aabb.max.x - worldPos.x,
		aabb.max.y - worldPos.y,
		aabb.max.z - worldPos.z);*/

	btVector3 halfExtents = btVector3(0.5f, 0.5f, 0.5f);

	if (type == Engine::ShapeType::BOX)
	{
		selectedCollider = physicsManager->AddBoxCollider(selectedEntity, center, btVector3(0.5f, 0.5f, 0.5f));		// Use half extents 0.5 so it is a unit cube so later we just use local scaling to get and set box size
		selectedCollider->SetBoxSize(worldScale);
	}
	else if (type == Engine::ShapeType::SPHERE)
	{
		selectedCollider = physicsManager->AddSphereCollider(selectedEntity, center, 1.0f);			// Same thing here use radius of 1

		float radius = glm::max(worldScale.x, glm::max(worldScale.y, worldScale.z));

		selectedCollider->SetRadius(radius);
	}
	else if (type == Engine::ShapeType::CAPSULE)
	{
		selectedCollider = physicsManager->AddCapsuleCollider(selectedEntity, center, 0.5f, 1.0f);			// Same thing as the other 2 above

		float radius = glm::max(worldScale.x, glm::max(worldScale.y, worldScale.z));

		selectedCollider->SetRadius(radius);
		selectedCollider->SetCapsuleHeight(worldScale.y);
	}

	SetEntity(selectedEntity);
	colliderOpen = true;

	editorManager->GetGizmo().SetSelectedEntity(selectedEntity);		// So the gizmo updates the selected entity collider, rigidbody or trigger
}

bool ObjectWindow::AddScript()
{
	for (size_t i = 0; i < files.size(); i++)
	{
		if (ImGui::Selectable(files[i].c_str()))
		{
			selectedScript = game->GetScriptManager().AddScript(selectedEntity, files[i]);

			if (selectedScript)
			{
#ifdef EDITOR
				selectedScript->CallOnAddEditorProperty(selectedEntity);
				selectedScript->RemovedUnusedProperties();
#endif

				if (selectedTrigger)
					selectedTrigger->SetScript(selectedScript);
			}


			SetEntity(selectedEntity);
			scriptOpen = true;

			return true;
		}
	}

	return false;
}

void ObjectWindow::AddLight(Engine::LightType type)
{
	if (type == Engine::LightType::SPOT)
	{
		//light = new SpotLight();
		//light->castShadows = false;
		//light->color = glm::vec3(1.0f);
	}
	else if (type == Engine::LightType::POINT)
	{
		selectedLight = game->GetLightManager().AddPointLight(selectedEntity);
		if (selectedLight)
		{
			Engine::PointLight *pl = static_cast<Engine::PointLight*>(selectedLight);
			pl->position = transformManager->GetLocalToWorld(selectedEntity)[3];
			game->GetLightManager().UpdatePointLights();
		}
	}

	SetEntity(selectedEntity);
	lightOpen = true;
}

void ObjectWindow::AddSoundSource()
{
	selectedSoundSource = game->GetSoundManager().AddSoundSource(selectedEntity);
	SetEntity(selectedEntity);
	soundSourceOpen = true;
}

void ObjectWindow::AddWidget(Engine::WidgetType type)
{
	switch (type)
	{
	case Engine::WidgetType::TEXT:
		selectedWidget = game->GetUIManager().AddText(selectedEntity);
		break;
	case Engine::WidgetType::BUTTON:
		selectedWidget = game->GetUIManager().AddButton(selectedEntity);
		break;
	case Engine::WidgetType::IMAGE:
		selectedWidget = game->GetUIManager().AddImage(selectedEntity);
		break;
	case Engine::WidgetType::EDIT_TEXT:
		selectedWidget = game->GetUIManager().AddEditText(selectedEntity);
		break;
	}

	SetEntity(selectedEntity);
}

void ObjectWindow::AddTexture()
{
	for (size_t i = 0; i < files.size(); i++)
	{
		if (ImGui::Selectable(files[i].c_str()))
		{
			Engine::Button *b = static_cast<Engine::Button*>(selectedWidget);

			Engine::TextureParams params = { Engine::TextureWrap::REPEAT, Engine::TextureFilter::LINEAR, Engine::TextureFormat::RGBA, Engine::TextureInternalFormat::SRGB8_ALPHA8, Engine::TextureDataType::UNSIGNED_BYTE, false, false };

			switch (choosenTextureType)
			{
			case 0:
				b->SetIdleTexture(game->GetRenderer()->CreateTexture2D(files[i], params));
				break;
			case 1:
				b->SetHoverTexture(game->GetRenderer()->CreateTexture2D(files[i], params));
				break;
			case 2:
				b->SetPressedTexture(game->GetRenderer()->CreateTexture2D(files[i], params));
				break;
			}
		}
	}
}

bool ObjectWindow::CreatePrefabFolder()
{
	std::string prefabDir = editorManager->GetCurrentProjectDir() + "/Prefabs";

	return Engine::utils::CreateDir(prefabDir.c_str());
}
