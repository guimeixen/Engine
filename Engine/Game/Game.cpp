#include "Game.h"

#include "Program/Utils.h"
#include "Program/StringID.h"
#include "Program/Input.h"
#include "Program/Log.h"
#include "Program/FileManager.h"
#include "Program/Version.h"

#include "Physics/RigidBody.h"
#include "Physics/Collider.h"
#include "Physics/Trigger.h"

#include "AI/AIObject.h"

#include "UI/UIManager.h"

#include "Graphics/ParticleSystem.h"
#include "Graphics/Renderer.h"
#include "Graphics/Model.h"
#include "Graphics/Material.h"
#include "Graphics/Effects/DebugDrawManager.h"
#include "Graphics/Animation/AnimatedModel.h"
#include "Graphics/Effects/ForwardRenderer.h"
#include "Graphics/Effects/ForwardPlusRenderer.h"
#include "Graphics/Effects/PSVitaRenderer.h"

#include "include/glm/gtx/quaternion.hpp"
#include "include/glm/gtc/matrix_transform.hpp"

#ifndef VITA
#include <Windows.h>
#endif

#include <cstdio>

namespace Engine
{
	Game::Game()
	{
		allocator = nullptr;
		renderer = nullptr;
		fileManager = nullptr;
		inputManager = nullptr;
		mainCamera = nullptr;
		fpsCamera = nullptr;
		terrain = nullptr;
		debugDrawManager = nullptr;
		renderingPath = nullptr;
		
		shouldShutdown = false;
		sceneChanged = false;
		sceneChangedScript = false;

		currentScene = 0;
		timeElapsed = 0.0f;
		deltaTime = 0.0f;

		playableWidth = 0.0f;
		playebleHeight = 0.0f;

		gameState = GameState::STOPPED;
	}

	void Game::Init(Allocator *allocator, Renderer *renderer, FileManager *fileManager, InputManager* inputManager)
	{
		Log::Print(LogLevel::LEVEL_INFO, "Starting Game\n");

		this->allocator = allocator;
		this->renderer = renderer;
		this->fileManager = fileManager;
		this->inputManager = inputManager;

		transformManager.Init(allocator, 50);
		scriptManager.Init(this);
		aiSystem.Init(this);
		physicsManager.Init(this);
		soundManager.Init(this, &transformManager);
		modelManager.Init(this, 50);	
		particleManager.Init(this);		
		uiManager.Init(this);

#ifdef VITA
		renderingPath = new PSVitaRenderer();
#else
		renderingPath = new ForwardRenderer();
#endif
		renderingPath->Init(this);
		lightManager.Init(this, &transformManager);

		// Pass a pointer in the bind call otherwise it will create a copy and might crash
		entityManager.AddComponentDestroyCallback(std::bind(&ScriptManager::RemoveScript, &scriptManager, std::placeholders::_1));
		entityManager.AddComponentDestroyCallback(std::bind(&PhysicsManager::RemoveRigidBody, &physicsManager, std::placeholders::_1));
		entityManager.AddComponentDestroyCallback(std::bind(&PhysicsManager::RemoveCollider, &physicsManager, std::placeholders::_1));
		entityManager.AddComponentDestroyCallback(std::bind(&PhysicsManager::RemoveTrigger, &physicsManager, std::placeholders::_1));
		entityManager.AddComponentDestroyCallback(std::bind(&SoundManager::RemoveSoundSource, &soundManager, std::placeholders::_1));
		entityManager.AddComponentDestroyCallback(std::bind(&ModelManager::RemoveModel, &modelManager, std::placeholders::_1));
		entityManager.AddComponentDestroyCallback(std::bind(&ParticleManager::RemoveParticleSystem, &particleManager, std::placeholders::_1));
		entityManager.AddComponentDestroyCallback(std::bind(&LightManager::RemoveLight, &lightManager, std::placeholders::_1));
		entityManager.AddComponentDestroyCallback(std::bind(&TransformManager::RemoveTransform, &transformManager, std::placeholders::_1));
		entityManager.AddComponentDestroyCallback(std::bind(&UIManager::RemoveWidget, &uiManager, std::placeholders::_1));

		entityManager.AddComponentDuplicateCallback(std::bind(&TransformManager::DuplicateTransform, &transformManager, std::placeholders::_1, std::placeholders::_2));		// Transform manager needs to go first
		entityManager.AddComponentDuplicateCallback(std::bind(&ScriptManager::DuplicateScript, &scriptManager, std::placeholders::_1, std::placeholders::_2));
		entityManager.AddComponentDuplicateCallback(std::bind(&PhysicsManager::DuplicateRigidBody, &physicsManager, std::placeholders::_1, std::placeholders::_2));
		entityManager.AddComponentDuplicateCallback(std::bind(&PhysicsManager::DuplicateCollider, &physicsManager, std::placeholders::_1, std::placeholders::_2));
		entityManager.AddComponentDuplicateCallback(std::bind(&PhysicsManager::DuplicateTrigger, &physicsManager, std::placeholders::_1, std::placeholders::_2));
		entityManager.AddComponentDuplicateCallback(std::bind(&SoundManager::DuplicateSoundSource, &soundManager, std::placeholders::_1, std::placeholders::_2));
		entityManager.AddComponentDuplicateCallback(std::bind(&ModelManager::DuplicateModel, &modelManager, std::placeholders::_1, std::placeholders::_2));
		entityManager.AddComponentDuplicateCallback(std::bind(&ParticleManager::DuplicateParticleSystem, &particleManager, std::placeholders::_1, std::placeholders::_2));
		entityManager.AddComponentDuplicateCallback(std::bind(&LightManager::DuplicatePointLight, &lightManager, std::placeholders::_1, std::placeholders::_2));
		entityManager.AddComponentDuplicateCallback(std::bind(&UIManager::DuplicateWidget, &uiManager, std::placeholders::_1, std::placeholders::_2));

		entityManager.AddComponentSetEnabledCallback(std::bind(&ScriptManager::SetScriptEnabled, &scriptManager, std::placeholders::_1, std::placeholders::_2));
		entityManager.AddComponentSetEnabledCallback(std::bind(&LightManager::SetPointLightEnabled, &lightManager, std::placeholders::_1, std::placeholders::_2));
		entityManager.AddComponentSetEnabledCallback(std::bind(&ModelManager::SetModelEnabled, &modelManager, std::placeholders::_1, std::placeholders::_2));
		entityManager.AddComponentSetEnabledCallback(std::bind(&ParticleManager::SetParticleSystemEnabled, &particleManager, std::placeholders::_1, std::placeholders::_2));
		entityManager.AddComponentSetEnabledCallback(std::bind(&PhysicsManager::SetRigidBodyEnabled, &physicsManager, std::placeholders::_1, std::placeholders::_2));
		entityManager.AddComponentSetEnabledCallback(std::bind(&PhysicsManager::SetColliderEnabled, &physicsManager, std::placeholders::_1, std::placeholders::_2));
		entityManager.AddComponentSetEnabledCallback(std::bind(&PhysicsManager::SetTriggerEnabled, &physicsManager, std::placeholders::_1, std::placeholders::_2));
		entityManager.AddComponentSetEnabledCallback(std::bind(&UIManager::SetWidgetEnabled, &uiManager, std::placeholders::_1, std::placeholders::_2));
		//entityManager.AddComponentSetEnabledCallback(std::bind(&SoundManager::SetSoundSourceEnabled, &soundManager, std::placeholders::_1, std::placeholders::_2));

		fpsCamera = new FPSCamera();
		fpsCamera->SetProjectionMatrix(70.0f, renderer->GetWidth(), renderer->GetHeight(), 0.2f, 700.0f);
		fpsCamera->SetPosition(glm::vec3(0.0f, 0.0f, 1.0f));
		fpsCamera->SetPitch(0.0f);
		fpsCamera->SetYaw(180.0f);
		//fpsCamera->SetFrontAndUp(glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, 1.0f, 0.0f));
		fpsCamera->SetMoveSpeed(2.0f);

		editorCam.SetPosition(glm::vec3(0.0f, 8.0f, 0.0f));
		editorCam.SetProjectionMatrix(70.0f, renderer->GetWidth(), renderer->GetHeight(), 0.2f, 700.0f);
		
#ifndef VITA
		debugDrawManager = new DebugDrawManager(renderer, scriptManager);
#endif
		renderer->AddRenderQueueGenerator(&uiManager);
		renderer->AddRenderQueueGenerator(&modelManager);
		renderer->AddRenderQueueGenerator(&particleManager);
#ifndef VITA
		renderer->AddRenderQueueGenerator(debugDrawManager);
#endif

#ifdef EDITOR
		renderingPath->SetMainCamera(&editorCam);
		mainCamera = &editorCam;
#else
		renderingPath->SetMainCamera(fpsCamera);
		mainCamera = fpsCamera;
		gameState = GameState::PLAYING;
#endif

		Log::Print(LogLevel::LEVEL_INFO, "Init Game\n");
	}

	void Game::Update(float dt)
	{
		transformManager.ClearModifiedTransforms();
		sceneChanged = false;
		deltaTime = dt;

		if (gameState != GameState::PAUSED)
		{
			timeElapsed += dt;
		}

		if (markedForLoadSceneID >= 0)
		{
			SetScene(markedForLoadSceneID, projectName);
			markedForLoadSceneID = -1;
		}

		modelManager.Update();
		soundManager.Update(mainCamera->GetPosition());
		//aiSystem.Update();
		
		if (gameState == GameState::PLAYING)
		{
#ifdef EDITOR
			fpsCamera->Update(deltaTime, true, true);
#else
			fpsCamera->Update(deltaTime, true, false);
#endif
			physicsManager.Simulate(deltaTime);
			physicsManager.Update();
			scriptManager.UpdateInGame(deltaTime);
			
			uiManager.UpdateInGame(deltaTime);
		}
		else if (gameState == GameState::PAUSED)
		{
			deltaTime = 0.0f;		
			physicsManager.Update();
			scriptManager.UpdateInGame(deltaTime);
			uiManager.UpdateInGame(deltaTime);
		}
		else
		{			
			physicsManager.Update();
			uiManager.Update(deltaTime);
		}
		
		// Editor camera. Update when the game is not running
		if (gameState == GameState::STOPPED)
		{
			editorCam.Update(deltaTime, true, true);
		}
	}

	void Game::Render(Renderer *renderer)
	{
#ifndef VITA
		//aiSystem.PrepareDebugDraw();
		physicsManager.PrepareDebugDraw(debugDrawManager);		
		debugDrawManager->Update();
#endif
		lightManager.Update(mainCamera);

		if (terrain)
		{
			if (terrain->IsEditable())
				terrain->UpdateEditing();

			terrain->UpdateVegColliders(mainCamera);
			terrain->UpdateLOD(mainCamera);
		}

		renderingPath->Render();

#ifndef VITA
		debugDrawManager->Clear();
#endif
	}

	void Game::PartialDispose()
	{
		if (terrain)
		{
			renderer->RemoveRenderQueueGenerator(terrain);
			terrain->Dispose();
			delete terrain;
			terrain = nullptr;
		}

		lightManager.PartialDispose();
		//aiSystem.Dispose();
		soundManager.Dispose();
		physicsManager.PartialDispose();
		//physicsManager.Dispose();
		scriptManager.PartialDispose();			// Do a full dispose to get rid of all the scripts loaded in Lua
		modelManager.PartialDispose();
		uiManager.PartialDispose();
	}

	void Game::Dispose()
	{
#ifndef VITA
		// Delete the temp file create by play mode
		std::remove("Data/temp.bin");
		std::remove("Data/uitemp.bin");
#endif

		Log::Print(LogLevel::LEVEL_INFO, "Disposing game\n");

		renderingPath->Dispose();
		delete renderingPath;

		Log::Print(LogLevel::LEVEL_INFO, "Rendering path disposed\n");

		if (fpsCamera)
			delete fpsCamera;

		Log::Print(LogLevel::LEVEL_INFO, "FPS Camera disposed\n");

		if (debugDrawManager)
			delete debugDrawManager;

		mainCamera = nullptr;		

		if (terrain)
		{
			renderer->RemoveRenderQueueGenerator(terrain);
			terrain->Dispose();
			delete terrain;
			terrain = nullptr;
		}

		particleManager.Dispose();
		lightManager.Dispose();
		aiSystem.Dispose();
		soundManager.Dispose();
		physicsManager.Dispose();
		scriptManager.Dispose();
		modelManager.Dispose();
		uiManager.Dispose();
		transformManager.Dispose();

		Log::Print(LogLevel::LEVEL_INFO, "Game disposed\n");
	}

	void Game::Resize(unsigned int width, unsigned int height)
	{
		playableWidth = width;
		playebleHeight = height;
		renderer->WaitIdle();
		renderingPath->Resize(width, height);
		fpsCamera->Resize(width, height);
		editorCam.Resize(width, height);
		uiManager.Resize(width, height);
	}

	bool Game::Save(const std::string &projectFolder, const std::string &projectName)
	{
#ifndef VITA
		if (utils::CreateDir("Data/Levels"))
		{
			if (!SaveProjectFile(projectFolder, projectName))
				return false;

			Serializer s(fileManager);
			s.OpenForWriting();

			s.Write(MAJOR_VERSION);
			s.Write(MINOR_VERSION);
			s.Write(PATCH_VERSION);

			entityManager.Serialize(s);
			transformManager.Serialize(s);
			lightManager.Serialize(s);
			modelManager.Serialize(s);
			particleManager.Serialize(s);
			soundManager.Serialize(s);
			scriptManager.Serialize(s);
			physicsManager.Serialize(s);
			uiManager.Serialize(s);

			s.Save(projectFolder + scenes[currentScene].name + ".bin");
			s.Close();

			if (terrain)
				terrain->Save(projectFolder, scenes[currentScene].name);

			renderingPath->SaveRenderingSettings("Data/Levels/" + projectName + '/' + scenes[currentScene].name + ".rendersettings");

			inputManager->SaveInputMappings(fileManager, projectFolder + "input.mappings");
		}
#endif
		return true;
	}

	bool Game::Load(const std::string &projectName)
	{
		if (!LoadProjectFile(projectName))
			return false;
		LoadTerrainFromFile(projectName, scenes[currentScene].name);
		LoadObjectsFromFile(projectName, scenes[currentScene].name);

		renderingPath->LoadRenderingSettings("Data/Levels/" + projectName + '/' + scenes[currentScene].name + ".rendersettings");

		// Fill the scripts properties
		// Must be done after all objects and widgets are loaded because an object's script might have properties using another object/widget
		/*const std::vector<Object*> &objects = sceneManager->GetObjects();
		for (size_t i = 0; i < objects.size(); i++)
		{
			Object *obj = objects[i];

			// Calling set layer again causes the objects to be updated in the physics world
			if (obj->GetLayer() == Layer::OBSTACLE)
				obj->SetLayer(Layer::OBSTACLE);

			AnimatedModel *am = obj->GetAnimatedModel();
			if (am)
			{
				am->FillBoneAttachments(this);
				am->UpdateBones(obj, 0.0f);		// Call the UpdateBones to set the bone transforms parents
			}
		}*/

		// Update the grid after we've loaded the objects
		//aiSystem.Init(this);

		// Not working. Even though we set the layer on OBSTACLE objects it still doesn't work. But clicking on rebuild immediate in the aiwindow works and does the exact same thing as we do here
		//aiSystem.GetAStarGrid().RebuildImmediate(this, aiSystem.GetAStarGrid().GetGridCenter());

		//aiSystem.GetAStarGrid().LoadGridFromFile();		// Should be done per scene eventually

		return true;
	}

	bool Game::LoadProject(const std::string &projectName)
	{
		if (!Load(projectName))
			return false;

		previousSceneId = currentScene;

		gameState = GameState::PLAYING;

		scriptManager.ReloadProperties();
		soundManager.Play();
		scriptManager.Play();
		physicsManager.Play();

		Log::Print(LogLevel::LEVEL_INFO, "Done loading project: %s\n", projectName.c_str());

		return true;
	}

	bool Game::SaveProjectFile(const std::string &projectFolder, const std::string &projectName)
	{
		this->projectName = projectName;
		projectDir = projectFolder;

		Serializer s(fileManager);
		s.OpenForWriting();

		s.Write(MAJOR_VERSION);
		s.Write(MINOR_VERSION);
		s.Write(PATCH_VERSION);

		s.Write(currentScene);
		
		s.Write(scenes.size());
		for (size_t i = 0; i < scenes.size(); i++)
		{
			s.Write(scenes[i].name);
		}

		s.Write(editorCam.GetPosition());
		s.Write(editorCam.GetFarPlane());
		s.Write(editorCam.GetPitch());
		s.Write(editorCam.GetYaw());
		s.Write(aiSystem.GetAStarGrid().GetGridCenter());
		s.Write(aiSystem.GetShowGrid());

		s.Save(projectFolder + '/' + projectName + ".proj");
		s.Close();

		return true;
	}

	bool Game::LoadProjectFile(const std::string &projectName)
	{
		this->projectName = projectName;
		projectDir = "Data/Levels/" + projectName + '/';

		glm::vec2 gridCenter;
		bool showGrid = false;

		Serializer s(fileManager);
		s.OpenForReading(projectDir + projectName + ".proj");
		if (s.IsOpen())
		{
			unsigned int major, minor, patch = 0;
			s.Read(major);
			s.Read(minor);
			s.Read(patch);

			if (major != MAJOR_VERSION || minor != MINOR_VERSION)
			{
				s.Close();
				return false;
			}

			s.Read(currentScene);
			previousSceneId = currentScene;

			unsigned int sceneCount = 0;
			s.Read(sceneCount);
			scenes.resize(static_cast<size_t>(sceneCount));
			for (size_t i = 0; i < scenes.size(); i++)
			{
				Scene scene = {};
				s.Read(scene.name);
				scenes[i] = scene;
			}

			glm::vec3 tempVec;
			float temp;

			s.Read(tempVec);
			editorCam.SetPosition(tempVec);
			fpsCamera->SetPosition(tempVec);

			s.Read(temp);

			editorCam.SetFarPlane(temp);
			fpsCamera->SetFarPlane(temp);

			s.Read(temp);

			editorCam.SetPitch(temp);
			fpsCamera->SetPitch(temp);

			s.Read(temp);

			editorCam.SetYaw(temp);
			fpsCamera->SetYaw(temp);

			s.Read(gridCenter);
			s.Read(showGrid);
		}
		s.Close();

		if (currentScene < 0 || currentScene >= (int)scenes.size())
			return false;


		//aiSystem.Init(this);
		aiSystem.GetAStarGrid().SetGridCenter(gridCenter);
		aiSystem.SetShowGrid(showGrid);

		return true;
	}

	void Game::LoadTerrainFromFile(const std::string &projectName, const std::string &sceneName)
	{
		std::ifstream terrainFile = fileManager->OpenForReading("Data/Levels/" + projectName + "/terrain_" + sceneName + ".dat");

		if (terrainFile.is_open())
		{
			std::string line;
			TerrainInfo info = {};

			while (std::getline(terrainFile, line))
			{
				if (line.substr(0, 4) == "mat=")
					info.matPath = line.substr(4);
				else if (line.substr(0, 4) == "veg=")
					info.vegPath = line.substr(4);
			}

			AddTerrain(info);
		}
	}

	void Game::LoadObjectsFromFile(const std::string &projectName, const std::string &sceneName)
	{
		Serializer s(fileManager);
		s.OpenForReading("Data/Levels/" + projectName + "/" + sceneName + ".bin");
		if (s.IsOpen())
		{
			unsigned int major, minor, patch = 0;
			s.Read(major);
			s.Read(minor);
			s.Read(patch);

			if (major == MAJOR_VERSION && minor == MINOR_VERSION)
			{
				entityManager.Deserialize(s);
				transformManager.Deserialize(s);
				lightManager.Deserialize(s);
				modelManager.Deserialize(s);
				particleManager.Deserialize(s);
				soundManager.Deserialize(s);
				scriptManager.Deserialize(s);
				physicsManager.Deserialize(s);
				uiManager.Deserialize(s);
			}
		}
		else
		{
			Log::Print(LogLevel::LEVEL_ERROR, "ERROR -> Failed to open scene file: %s.bin\n", sceneName.c_str());
		}
		s.Close();
	}

	void Game::SaveBeforePlayMode()
	{
		Serializer s(fileManager);
		s.OpenForWriting();

		entityManager.Serialize(s);
		transformManager.Serialize(s);
		lightManager.Serialize(s, true);
		modelManager.Serialize(s, true);
		particleManager.Serialize(s, true);
		soundManager.Serialize(s, true);
		scriptManager.Serialize(s, true);
		physicsManager.Serialize(s, true);
		uiManager.Serialize(s, true);

		s.Save("Data/temp.bin");
		s.Close();
	}

	void Game::RevertPlayMode()
	{
		Serializer s(fileManager);
		s.OpenForReading("Data/temp.bin");
		if (s.IsOpen())
		{
			entityManager.Deserialize(s);
			transformManager.Deserialize(s);
			lightManager.Deserialize(s, true);
			modelManager.Deserialize(s, true);
			particleManager.Deserialize(s, true);
			soundManager.Deserialize(s, true);
			scriptManager.Deserialize(s, true);
			physicsManager.Deserialize(s, true);
			uiManager.Deserialize(s, true);
		}
		s.Close();
	}

	void Game::Play()
	{
		if (gameState == GameState::PLAYING)
			return;

		if (gameState == GameState::PAUSED)
		{
			gameState = GameState::PLAYING;
			mainCamera->Reset();
			return;
		}

		// Store the scene we're in, so we know if we changed it when we stop
		previousSceneId = currentScene;

		SaveBeforePlayMode();

		gameState = GameState::PLAYING;

		renderingPath->SetMainCamera(fpsCamera);
		mainCamera = fpsCamera;

		soundManager.Play();
		scriptManager.ReloadScripts();
		scriptManager.Play();
		physicsManager.Play();
	}

	void Game::Pause()
	{
		if (gameState == GameState::PAUSED || gameState == GameState::STOPPED)
			return;

		gameState = GameState::PAUSED;
	}

	void Game::Stop()
	{
		if (gameState == GameState::STOPPED)
			return;

		renderingPath->SetMainCamera(&editorCam);
		mainCamera = &editorCam;

		// Reload the scene if another one was loaded through script
		if (currentScene != previousSceneId && sceneChangedScript)
		{
			SetScene(previousSceneId, projectName);
			sceneChangedScript = false;
		}

		// Make sure to clear forces and velocities on the rigid bodies, otherwise when playing again they will start with the velocity they had when stopped
		physicsManager.Stop();

		// Load the scene first (above and only if it was changed) and only then revert the changes made while in play mode
		RevertPlayMode();

		/*const std::vector<Object*> &objects = sceneManager->GetObjects();
		for (size_t i = 0; i < objects.size(); i++)
		{
			AnimatedModel *am = objects[i]->GetAnimatedModel();
			if (am)
				am->FillBoneAttachments(this);
		}*/

		gameState = GameState::STOPPED;

#ifndef VITA
		std::remove("Data/temp.bin");
		std::remove("Data/uitemp.bin");
#endif
	}

	Entity Game::AddEntity()
	{
		Entity e = entityManager.Create();
		transformManager.AddTransform(e);
#ifdef EDITOR
		transformManager.SetLocalPosition(e, editorCam.GetPosition() + editorCam.GetFront() * 4.0f);
#endif
		return e;
	}

	Entity Game::DuplicateEntity(Entity e)
	{
		return entityManager.Duplicate(e);
	}

	void Game::SetEntityEnabled(Entity e, bool enable)
	{
		entityManager.SetEnabled(e, enable);

		// Also enable/disabled the children
		Entity child = transformManager.GetFirstChild(e);
		while (child.IsValid())
		{
			entityManager.SetEnabled(child, enable);
			child = transformManager.GetNextSibling(child);
		}
	}

	void Game::DestroyEntity(Entity e)
	{
		entityManager.Destroy(e);
	}

	void Game::SaveEntityPrefab(Entity e, const std::string &path)
	{
		if (!e.IsValid())
			return;

		Serializer s(fileManager);
		s.OpenForWriting();
		SaveEntityPrefabRecursively(s, e);
		s.Save(path);
		s.Close();
	}

	void Game::SaveEntityPrefabRecursively(Serializer &s, Entity e)
	{
		s.Write(transformManager.GetLocalPosition(e));
		s.Write(transformManager.GetLocalRotation(e));
		s.Write(transformManager.GetLocalScale(e));

		bool hasScript = scriptManager.HasScript(e);
		s.Write(hasScript);
		if (hasScript)
			scriptManager.GetScript(e)->Serialize(s);

		bool hasRb = physicsManager.HasRigidBody(e);
		s.Write(hasRb);
		if (hasRb)
			physicsManager.GetRigidBody(e)->Serialize(s);

		bool hasCol = physicsManager.HasCollider(e);
		s.Write(hasCol);
		if (hasCol)
			physicsManager.GetCollider(e)->Serialize(s);

		bool hasTr = physicsManager.HasTrigger(e);
		s.Write(hasTr);
		if (hasTr)
			physicsManager.GetTrigger(e)->Serialize(s);

		// Sound source

		if (modelManager.HasAnimatedModel(e) || modelManager.HasModel(e))
		{
			s.Write(true);
			modelManager.SaveModelPrefab(s, e);
		}
		else
		{
			s.Write(false);
		}

		bool hasPs = particleManager.HasParticleSystem(e);
		s.Write(hasPs);
		if (hasPs)
			particleManager.GetParticleSystem(e)->Serialize(s);

		bool hasPl = lightManager.HasPointLight(e);
		s.Write(hasPl);
		if (hasPl)
			lightManager.GetPointLight(e)->Serialize(s);

		bool hasWidget = uiManager.HasWidget(e);
		s.Write(hasWidget);
		if (hasWidget)
			uiManager.GetWidget(e)->Serialize(s);

		// Loop through every children
		s.Write(transformManager.HasChildren(e));

		// First save the number of children and then the actual children
		unsigned short numChildren = 0;
		Entity child = transformManager.GetFirstChild(e);
		while (child.IsValid())
		{
			numChildren++;
			child = transformManager.GetNextSibling(child);
		}
		s.Write(numChildren);

		child = transformManager.GetFirstChild(e);
		while (child.IsValid())
		{
			SaveEntityPrefabRecursively(s, child);
			child = transformManager.GetNextSibling(child);
		}
	}

	Entity Game::LoadEntityPrefab(const std::string &path)
	{
		Entity e = entityManager.Create();
		transformManager.AddTransform(e);

		Serializer s(fileManager);
		s.OpenForReading(path);

		if (s.IsOpen())
		{
			LoadEntityPrefabRecursively(s, e);
			s.Close();
		}
		else
		{
			Log::Print(LogLevel::LEVEL_ERROR, "ERROR -> Failed to load prefab: %s\n", path.c_str());
		}

		// Place the entity in front of the camera
#ifdef EDITOR
		transformManager.SetLocalPosition(e, editorCam.GetPosition() + editorCam.GetFront() * 4.0f);
#endif
		
		return e;
	}

	void Game::LoadEntityPrefabRecursively(Serializer &s, Entity e)
	{
		glm::vec3 localPos, localScale;
		glm::quat localRot;

		s.Read(localPos);
		s.Read(localRot);
		s.Read(localScale);

		transformManager.SetLocalPosition(e, localPos);
		transformManager.SetLocalRotation(e, localRot);
		transformManager.SetLocalScale(e, localScale);

		bool hasScript, hasRb, hasCol, hasTr, hasModel, hasPs, hasPl, hasWidget = false;

		s.Read(hasScript);
		if (hasScript)
			scriptManager.LoadFromPrefab(s, e);

		s.Read(hasRb);
		if (hasRb)
			physicsManager.LoadRigidBodyFromPrefab(s, e);

		s.Read(hasCol);
		if (hasCol)
			physicsManager.LoadColliderFromPrefab(s, e);

		s.Read(hasTr);
		if (hasTr)
			physicsManager.LoadTriggerFromPrefab(s, e);

		s.Read(hasModel);
		if (hasModel)
			modelManager.LoadModelPrefab(s, e);

		s.Read(hasPs);
		if (hasPs)
			particleManager.LoadParticleSystemFromPrefab(s, e);

		s.Read(hasPl);
		if (hasPl)
			lightManager.LoadPointLightFromPrefab(s, e);

		s.Read(hasWidget);
		if (hasWidget)
			uiManager.LoadWidgetFromPrefab(s, e);

		bool hasChildren = false;
		s.Read(hasChildren);

		if (hasChildren)
		{
			unsigned short numChildren = 0;
			s.Read(numChildren);

			for (unsigned short i = 0; i < numChildren; i++)
			{
				Entity child = entityManager.Create();
				transformManager.AddTransform(child);
				transformManager.SetParent(child, e);

				LoadEntityPrefabRecursively(s, child);		
			}
		}
	}

	void Game::AddTerrain(const TerrainInfo &info)
	{
		if (terrain)
			return;

		terrain = new Terrain();

		if (terrain)
		{
			if (terrain->Init(this, info))
			{
				renderer->AddRenderQueueGenerator(terrain);
#ifdef EDITOR
				renderingPath->EnableTerrainEditing();
#endif
			}
		}
		else
			Log::Print(LogLevel::LEVEL_ERROR, "ERROR -> Failed to create terrain\n");
	}

	bool Game::PerformRayIntersection(const glm::vec2 &point, Entity &outEntity)
	{
		return modelManager.PerformRaycast(mainCamera, point, outEntity);
	}

	void Game::SetScene(int sceneId, const std::string &projectName)
	{
		if (sceneId < 0 || sceneId >= (int)scenes.size())
			return;

		PartialDispose();
		currentScene = sceneId;

		soundManager.Init(this, &transformManager);

		LoadTerrainFromFile(projectName, scenes[currentScene].name);
		LoadObjectsFromFile(projectName, scenes[currentScene].name);

		// Update the grid after we've loaded the objects
		//physicsManager.Update(0.01f);
		//aiSystem.Init(this);
		//aiSystem.GetAStarGrid().RebuildImmediate(this, aiSystem.GetAStarGrid().GetGridCenter());

		// Fill the scripts properties
		// Must be done after all objects and widgets are loaded because an object's script might have properties using another object/widget
		/*const std::vector<Object*> &objects = sceneManager->GetObjects();
		for (size_t i = 0; i < objects.size(); i++)
		{
			Object *obj = objects[i];

			AnimatedModel *am = obj->GetAnimatedModel();
			if (am)
			{
				am->FillBoneAttachments(this);
				am->UpdateBones(obj, 0.0f);		// Call the UpdateBones to set the bone transforms parents
			}
		}*/

		if (gameState == GameState::PLAYING)		// If SetScene() is being called in play mode then we need to reload all properties because ReloadProperties() only gets called on we click the Play button
		{
			//scriptManager.ReloadScripts();			// No need to call ReloadScripts because they were loaded through LoadObjects and uiManager->Load() so we just need to load the properties

			scriptManager.ReloadProperties();
			scriptManager.Play();				// Call oninit
			physicsManager.Play();				// Activate rbs
		}

		editorCam.SetPosition(glm::vec3());
	}

	void Game::AddScene(const std::string &name)
	{
		Scene s = {};
		s.name = name;
		scenes.push_back(s);
	}

	void Game::SetSceneScript(const std::string &sceneName)
	{
		if (sceneName == scenes[currentScene].name)
			return;

		for (size_t i = 0; i < scenes.size(); i++)
		{
			if (sceneName == scenes[i].name)
			{
				//previousSceneId = currentScene;
				currentScene = i;
				sceneChangedScript = true;
				sceneChanged = true;
				markedForLoadSceneID = i;			// We need to mark the scene for load because if we load it right now it would cause a crash when we delete the scripts
													// because the one that called this function would not be finished executing all the luabridge code
				break;
			}
		}
	}

	void Game::Print(const char* str)
	{
		Log::Print(LogLevel::LEVEL_INFO, str);
	}
}
