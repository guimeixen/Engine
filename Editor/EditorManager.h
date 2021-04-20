#pragma once

#include "Physics/BoundingVolumes.h"
#include "Physics/Ray.h"
#include "Program/PSVCompiler.h"
#include "Program/Input.h"

#include "imgui/imgui.h"

#include "ObjectWindow.h"
#include "Commands.h"
#include "SceneWindow.h"
#include "Gizmo.h"
#include "TerrainWindow.h"
#include "MaterialWindow.h"
#include "AIWindow.h"
#include "RenderingWindow.h"
#include "AnimationWindow.h"
#include "AnimationProperties.h"
#include "SkeletonTreeWindow.h"
#include "ConsoleWindow.h"
#include "AssetsBrowserWindow.h"
#include "EditorNameManager.h"

#include <stack>

struct GLFWwindow;

class EditorManager
{
public:
	EditorManager();

	void Init(GLFWwindow *window, Engine::Game *game, Engine::InputManager *inputManager);
	void Update(float dt);
	void Render();
	void Dispose();

	void UpdateKeys(int key, int action, int mods);
	void UpdateScroll(double yoffset);
	void UpdateMouse(int button, int action);
	void UpdateChar(unsigned int c);
	void OnWindowClose();
	void OnFocus();

	std::stack<EditorCommand*> &GetUndoStack() { return undoStack; }

	bool IsProjectOpen() const { return isProjectOpen; }

	//std::string GetCurrentLevelName() const { return std::string(projectName); }

	// Return Data/Levels/project_name  Does not put the last slash
	const std::string &GetCurrentProjectDir() const { return curLevelDir; }

	Gizmo &GetGizmo() { return gizmo; }
	SceneWindow &GetSceneWindow() { return sceneWindow; }
	ObjectWindow &GetObjectWindow() { return objectWindow; }
	MaterialWindow &GetMaterialWindow() { return materialWindow; }
	AnimationWindow &GetAnimationWindow() { return animationWindow; }

	EditorNameManager &GetEditorNameManager() { return editorNameManager; }

	glm::vec2 GetGameViewSize() { return glm::vec2(gameViewSize.x, gameViewSize.y); }
	glm::vec2 GetGameViewportPos() { return glm::vec2(viewportPos.x, viewportPos.y); }
	bool IsMouseInsideGameView() const;

	bool WasProjectJustLoaded() const { return projectJustLoaded; }

	// Script functions
	void AddEntityScriptProperty(const std::string &propName, Engine::Entity eWithProp);

private:
	void ShowMainMenuBar();
	void ShowGameViewToolBar();
	ImVec2 HandleGameView();
	void HandleObjectWindow();
	void HandleSceneWindow();
	void HandleProjectCreation();

	void DisplayLoadPopup();
	void SaveAs();
	void ExitPopup();
	void UnsavedWarning();

	void FindFilesInDirectory(const char *dir);

	void SaveProject();
	void Undo();
	void Redo();

private:
	Engine::Game *game;
	Engine::InputManager *inputManager;
	GLFWwindow *window;

	ObjectWindow objectWindow;
	SceneWindow sceneWindow;
	MaterialWindow materialWindow;
	TerrainWindow terrainWindow;
	AIWindow aiWindow;
	RenderingWindow renderingWindow;
	AnimationWindow animationWindow;
	AnimationProperties animationPropsWindow;
	SkeletonTreeWindow skeletonWindow;
	ConsoleWindow consoleWindow;
	AssetsBrowserWindow assetsBrowserWindow;
	Gizmo gizmo;

	Engine::PSVCompiler psvCompiler;

	EditorNameManager editorNameManager;

	std::stack<EditorCommand*> undoStack;
	std::stack<EditorCommand*> redoStack;

	std::vector<std::string> files;
	bool addingPrefab = false;

	int mainMenuHeight;
	bool showGameView = true;

	char projectName[256];
	std::string curLevelDir;
	bool needsSaving = false;
	bool needsProjectCreation = true;
	bool needsLoading = false;
	std::vector<std::string> folderInLevelFolder;

	bool firstLevelSave = true;
	bool showExitPopup = false;
	bool showUnsavedWarning = false;
	bool projectJustLoaded = false;
	bool isProjectOpen = false;
	bool displayProjectLoadErrorPopup = false;
	bool displayAddInputMappingPopup;

	Engine::InputMapping newInputMapping;
	char inputMappingName[64];

	ImVec2 availableSize;
	ImVec2 gameViewSize;
	glm::vec2 viewportPos;
	float camSpeed;

	int currentScene = 0;

	bool clickBeganOnGameView = false;

	// ImGui input
	bool mousePressed[3] = { false, false, false };
	float mouseWheel = 0.0f;

	// Vita
	char vitaAppName[128];
	char vitaAppTitleID[10];

	// Changing input mapping
	bool isInputMappingWindowOpen;
	int changingInputMappingIndex;
	bool changingPositiveKey;
	bool changingNegativeKey;
	bool changingPositiveVitaButton;
	bool changingNegativeVitaButton;
	int currentPositiveKeyIndex;
};

