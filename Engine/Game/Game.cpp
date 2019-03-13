#include "Game.h"

#include "Program\Utils.h"
#include "Program\StringID.h"
#include "Program\Input.h"
#include "Program\Log.h"

#include "Physics\RigidBody.h"
#include "Physics\Collider.h"
#include "Physics\Trigger.h"

#include "AI\AIObject.h"

#include "UI\UIManager.h"

#include "Graphics\Renderer.h"
#include "Graphics\Model.h"
#include "Graphics\Material.h"
#include "Graphics\ResourcesLoader.h"
#include "Graphics\Effects\DebugDrawManager.h"
#include "Graphics\ResourcesLoader.h"
#include "Graphics\Animation\AnimatedModel.h"
#include "Graphics/Effects/ForwardRenderer.h"
#include "Graphics/Effects/ForwardPlusRenderer.h"

#include "include\glm\gtx\quaternion.hpp"
#include "include\glm\gtc\matrix_transform.hpp"

#include <Windows.h>

#include <iostream>
#include <cstdio>

namespace Engine
{
	Game::Game()
	{
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

		gameState = GameState::STOPPED;
	}

	void Game::Init(Renderer *renderer)
	{
		this->renderer = renderer;

		transformManager.Init(50);
		scriptManager.Init(this);
		aiSystem.Init(this);
		physicsManager.Init(&transformManager);
		soundManager.Init(this, &transformManager);
		modelManager.Init(this, 50);	
		particleManager.Init(this);		
		uiManager.Init(this);

		renderingPath = new ForwardPlusRenderer();
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

		fpsCamera = new FPSCamera();
		fpsCamera->SetProjectionMatrix(70.0f, renderer->GetWidth(), renderer->GetHeight(), 0.2f, 700.0f);
		editorCam.SetProjectionMatrix(70.0f, renderer->GetWidth(), renderer->GetHeight(), 0.2f, 700.0f);
		fpsCamera->SetMoveSpeed(32.0f);

		debugDrawManager = new DebugDrawManager(renderer, scriptManager);

		renderer->AddRenderQueueGenerator(&uiManager);
		renderer->AddRenderQueueGenerator(debugDrawManager);
		renderer->AddRenderQueueGenerator(&modelManager);
		renderer->AddRenderQueueGenerator(&particleManager);

#ifdef EDITOR
		renderingPath->SetMainCamera(&editorCam);
		mainCamera = &editorCam;
#else
		renderingPath->SetMainCamera(fpsCamera);
		mainCamera = fpsCamera;
		gameState = GameState::PLAYING;
#endif
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
			fpsCamera->Update(deltaTime, true, true);
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
		//aiSystem.PrepareDebugDraw();
		physicsManager.PrepareDebugDraw(debugDrawManager);		
		debugDrawManager->Update();
		lightManager.Update(mainCamera);

		if (terrain)
		{
			terrain->UpdateVegColliders(mainCamera);
			terrain->UpdateLOD(mainCamera);
		}

		renderingPath->Render();
		debugDrawManager->Clear();
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
		// Delete the temp file create by play mode
		std::remove("Data/temp.bin");
		std::remove("Data/uitemp.bin");

		renderingPath->Dispose();

		if (fpsCamera)
			delete fpsCamera;
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
		bool canSave = false;

		if (CreateDirectory((LPCWSTR)"Data/Levels", NULL))
			canSave = true;
		else if (ERROR_ALREADY_EXISTS == GetLastError())
			canSave = true;
		else
		{
			std::cout << "Failed to create levels directory. Error: " << GetLastError() << '\n';
			return false;
		}

		if (canSave)
		{
			if (!SaveProjectFile(projectFolder, projectName))
				return false;

			Serializer s;
			s.OpenForWriting();

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
		}

		return true;
	}

	bool Game::Load(const std::string &projectName)
	{
		if (!LoadProjectFile(projectName))
			return false;
		LoadTerrainFromFile(projectName, scenes[currentScene].name);
		LoadObjectsFromFile(projectName, scenes[currentScene].name);

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

		aiSystem.GetAStarGrid().LoadGridFromFile();		// Should be done per scene eventually

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

		return true;
	}

	bool Game::SaveProjectFile(const std::string &projectFolder, const std::string &projectName)
	{
		this->projectName = projectName;
		projectDir = projectFolder;

		// Save the proj file first
		std::ofstream projFile(projectFolder + '/' + projectName + ".proj");

		if (!projFile.is_open())
		{
			Log::Print(LogLevel::LEVEL_ERROR, "Failed to open project file for saving!");
			return false;
		}

		// Save the current scene post process file
		//mainView->SavePostProcessFile(projectFolder + '/' + scenes[currentScene].name + "_pp.data");

		glm::vec3 pos = editorCam.GetPosition();
		projFile << "curScene=" << currentScene << '\n';
		for (size_t i = 0; i < scenes.size(); i++)
		{
			projFile << "scene=" << scenes[i].name << '\n';

		}
		projFile << "pos=" << pos.x << ' ' << pos.y << ' ' << pos.z << '\n';
		projFile << "farPlane=" << editorCam.GetFarPlane() << '\n';
		projFile << "pitch=" << editorCam.GetPitch() << '\n';
		projFile << "yaw=" << editorCam.GetYaw() << '\n';
		glm::vec2 gridCenter = aiSystem.GetAStarGrid().GetGridCenter();
		projFile << "aiGridCenter=" << gridCenter.x << ' ' << gridCenter.y << '\n';
		projFile << "aiShowGrid=" << aiSystem.GetShowGrid() << '\n';
		projFile.close();

		return true;
	}

	bool Game::LoadProjectFile(const std::string &projectName)
	{
		this->projectName = projectName;
		projectDir = "Data/Levels/" + projectName + "/";

		// Load the proj file first
		std::ifstream projFile(projectDir + projectName + ".proj");

		std::string line;
		glm::vec3 position;

		if (!projFile.is_open())
		{
			std::cout << "Error! Failed to load project file: " << projectName << '\n';
			return false;
		}

		glm::vec2 gridCenter;
		bool showGrid = false;
		std::string sceneNameTemp;
		//DirLight mainLight;

		while (std::getline(projFile, line))
		{
			if (line.substr(0, 9) == "curScene=")
			{
				currentScene = std::stoi(line.substr(9));
				previousSceneId = currentScene;
			}
			else if (line.substr(0, 6) == "scene=")
			{
				Scene s = {};
				s.name = line.substr(6);
				scenes.push_back(s);
			}
			else if (line.substr(0, 4) == "pos=")		// id = 0 reserved for root node
			{
				std::istringstream ss(line.substr(4));
				ss >> position.x >> position.y >> position.z;

				editorCam.SetPosition(position);
				fpsCamera->SetPosition(position);
			}
			else if (line.substr(0, 9) == "farPlane=")
			{
				float farPlane = std::stof(line.substr(9));
				editorCam.SetFarPlane(farPlane);
				fpsCamera->SetFarPlane(farPlane);
			}
			else if (line.substr(0, 6) == "pitch=")
			{
				float pitch = std::stof(line.substr(6));
				editorCam.SetPitch(pitch);
				fpsCamera->SetPitch(pitch);
			}
			else if (line.substr(0, 4) == "yaw=")
			{
				float yaw = std::stof(line.substr(4));
				editorCam.SetYaw(yaw);
				fpsCamera->SetYaw(yaw);
			}
			else if (line.substr(0, 13) == "aiGridCenter=")
			{
				std::istringstream ss(line.substr(13));
				ss >> gridCenter.x >> gridCenter.y;
			}
			else if (line.substr(0, 11) == "aiShowGrid=")
			{
				int show = std::stoi(line.substr(11));
				if (show == 1)
					showGrid = true;
				else
					showGrid = false;
			}
		}

		projFile.close();

		if (currentScene < 0 || currentScene >= (int)scenes.size())
			return false;

		
		//aiSystem.Init(this);
		aiSystem.GetAStarGrid().SetGridCenter(gridCenter);
		aiSystem.SetShowGrid(showGrid);

		return true;
	}

	void Game::LoadTerrainFromFile(const std::string &projectName, const std::string &sceneName)
	{
		// Try and open the terrain cfg file
		std::ifstream terrainFile("Data/Levels/" + projectName + "/terrain_" + sceneName + ".dat");

		if (terrainFile.is_open())
		{
			std::string line;
			TerrainInfo info = {};

			while (std::getline(terrainFile, line))
			{
				if (line.substr(0, 4) == "mat=")
				{
					info.matPath = line.substr(4);
				}
				else if (line.substr(0, 4) == "veg=")
				{
					info.vegPath = line.substr(4);
				}
			}

			AddTerrain(info);

			terrainFile.close();
		}
	}

	void Game::LoadObjectsFromFile(const std::string &projectName, const std::string &sceneName)
	{
		Serializer s;
		s.OpenForReading("Data/Levels/" + projectName + "/" + sceneName + ".bin");
		if (s.IsOpen())
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
		s.Close();
	}

	void Game::SaveBeforePlayMode()
	{
		Serializer s;
		s.OpenForWriting();

		entityManager.Serialize(s);
		transformManager.Serialize(s);
		lightManager.Serialize(s);
		modelManager.Serialize(s);
		particleManager.Serialize(s);
		soundManager.Serialize(s);
		scriptManager.Serialize(s);
		physicsManager.Serialize(s);
		uiManager.Serialize(s);

		s.Save("Data/temp.bin");
		s.Close();
	}

	void Game::RevertPlayMode()
	{
		Serializer s;
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

		std::remove("Data/temp.bin");
		std::remove("Data/uitemp.bin");
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

	void Game::DestroyEntity(Entity e)
	{
		entityManager.Destroy(e);
	}

	void Game::SaveEntityPrefab(const std::string &path)
	{
	}

	void Game::LoadEntityPrefab(const std::string &path)
	{
	}

	void Game::AddTerrain(const TerrainInfo &info)
	{
		if (terrain)
			return;

		terrain = new Terrain();

		if (terrain)
		{
			if (terrain->Init(this, info))
				renderer->AddRenderQueueGenerator(terrain);
		}
		else
			std::cout << "Failed to create terrain\n";
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
}
