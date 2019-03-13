#pragma once

#include "EntityManager.h"
#include "ComponentManagers\ModelManager.h"
#include "ComponentManagers\TransformManager.h"
#include "ComponentManagers\ParticleManager.h"
#include "ComponentManagers\PhysicsManager.h"
#include "ComponentManagers\ScriptManager.h"
#include "ComponentManagers\LightManager.h"
#include "ComponentManagers\SoundManager.h"
#include "UI\UIManager.h"

#include "Graphics/Camera/FPSCamera.h"
#include "AI/AISystem.h"
#include "Graphics/Terrain/Terrain.h"
#include "Graphics/Effects/RenderingPath.h"

namespace Engine
{
	enum class GameState
	{
		PLAYING,
		PAUSED,
		STOPPED
	};

	class Renderer;
	class Terrain;
	class MainView;
	class DebugDrawManager;

	struct Scene
	{
		std::string name;
	};

	class Game
	{
	public:
		Game();

		void Init(Renderer *renderer);
		void Update(float dt);
		void Render(Renderer *renderer);
		// Used when creating a new or opening a project. It just disposes what is not necessary
		void PartialDispose();
		void Dispose();

		void Resize(unsigned int width, unsigned int height);

		bool Save(const std::string &projectFolder, const std::string &projectName);
		bool Load(const std::string &projectName);
		bool LoadProject(const std::string &projectName);

		void SaveBeforePlayMode();
		void RevertPlayMode();

		void Play();
		void Pause();
		void Stop();
		bool IsPlaying() const { return gameState == GameState::PLAYING; }
		GameState GetState() const { return gameState; }

		float GetTimeElapsed() const { return timeElapsed; }
		float GetDeltaTime() const { return deltaTime; }
		unsigned int GetPlayableWidth() const { return playableWidth; }
		unsigned int GetPlayebleHeight() const { return playebleHeight; }

		Entity AddEntity();
		Entity DuplicateEntity(Entity e);
		void DestroyEntity(Entity e);		
		void SaveEntityPrefab(const std::string &path);
		void LoadEntityPrefab(const std::string &path);

		void AddTerrain(const TerrainInfo &info);

		void SetScene(int sceneId, const std::string &projectName);
		void AddScene(const std::string &name);
		int GetCurrentSceneId() const { return currentScene; }

		const std::string &GetProjectName() const { return projectName; }
		const std::string &GetProjectDir() const { return projectDir; }
		const std::vector<Scene> &GetScenes() const { return scenes; }

		DebugDrawManager*		GetDebugDrawManager() const { return debugDrawManager; }
		Renderer*				GetRenderer() const { return renderer; }
		AISystem&				GetAISystem() { return aiSystem; }
		Terrain*				GetTerrain() const { return terrain; }

		EntityManager&			GetEntityManager() { return entityManager; }
		ModelManager&			GetModelManager() { return modelManager; }
		TransformManager&		GetTransformManager() { return transformManager; }
		PhysicsManager&			GetPhysicsManager() { return physicsManager; }
		ParticleManager&		GetParticleManager() { return particleManager; }
		ScriptManager&			GetScriptManager() { return scriptManager; }
		LightManager&			GetLightManager() { return lightManager; }
		SoundManager&			GetSoundManager() { return soundManager; }
		UIManager&				GetUIManager() { return uiManager; }

		RenderingPath*			GetRenderingPath() { return renderingPath; }

		FPSCamera &GetEditorCamera() { return editorCam; }

		bool PerformRayIntersection(const glm::vec2 &point, Entity &outEntity);

		bool HasSceneChanged() const { return sceneChangedScript; }
		bool ShouldShutdown() const { return shouldShutdown; }

		// Script functions
		FPSCamera *GetMainCamera() const { return mainCamera; }
		void SetSceneScript(const std::string &sceneName);
		void Shutdown() { shouldShutdown = true; }

		bool IsPaused() const { return isPaused; }

	private:
		bool SaveProjectFile(const std::string &projectFolder, const std::string &projectName);
		bool LoadProjectFile(const std::string &projectName);
		void LoadTerrainFromFile(const std::string &projectName, const std::string &sceneName);
		void LoadObjectsFromFile(const std::string &projectName, const std::string &sceneName);

	private:
		std::string projectName;
		std::string projectDir;
		bool shouldShutdown;
		unsigned int playableWidth;
		unsigned int playebleHeight;

		Renderer			*renderer;		
		AISystem			aiSystem;
		Terrain				*terrain;
		DebugDrawManager	*debugDrawManager;

		RenderingPath		*renderingPath;

		EntityManager		entityManager;
		TransformManager	transformManager;
		ModelManager		modelManager;
		LightManager		lightManager;		
		ParticleManager		particleManager;
		ScriptManager		scriptManager;
		PhysicsManager		physicsManager;
		SoundManager		soundManager;
		UIManager			uiManager;

		std::vector<Scene> scenes;
		int currentScene;

		float timeElapsed;
		float deltaTime;
		GameState gameState;

		FPSCamera *fpsCamera;
		FPSCamera editorCam;
		FPSCamera *mainCamera;

		bool sceneChanged;
		bool sceneChangedScript;		// Needed so the editor knows we've changed scene, if it was changed through script
		int previousSceneId = -1;
		int markedForLoadSceneID = -1;

		bool isPaused = false;
	};
}
