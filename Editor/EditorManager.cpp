#include "EditorManager.h"

#include "Game\Game.h"
#include "Game\UI\UIManager.h"

#include "Program\Input.h"
#include "Program\Utils.h"
#include "Program\Log.h"
#include "Program\Serializer.h"

#include "Graphics\Renderer.h"
#include "Graphics\Model.h"
#include "Graphics\Material.h"
#include "Graphics\ResourcesLoader.h"
#include "Graphics\Effects\MainView.h"

#include "Physics\RigidBody.h"

#include <GLFW\glfw3.h>

#include <include\glm\gtc\matrix_transform.hpp>

#include "imgui\imgui_dock.h"
#include "imgui\imgui_impl_glfw.h"
#include "imgui\imgui_impl_opengl3.h"
#include "imgui\imgui_impl_vulkan.h"
#include "Graphics\VK\VKRenderer.h"

#ifdef _WIN32
#define GLFW_EXPOSE_NATIVE_WIN32
#include "GLFW\glfw3native.h"
#include "imgui\imgui_impl_dx11.h"
#include "Graphics\D3D11\D3D11Renderer.h"
#include <Windows.h>
#endif

#include <iostream>
#include <fstream>
#include <sstream>
#include <chrono>
#include <filesystem>

EditorManager::EditorManager()
{
	gameViewSize = ImVec2();
	availableSize = ImVec2();
	std::memset(projectName, 0, 256);
}

EditorManager::~EditorManager()
{
}

void EditorManager::Init(GLFWwindow *window, Engine::Game *game)
{
	this->window = window;
	ImGui::CreateContext();
	ImGuiStyle &style = ImGui::GetStyle();
	style.FrameBorderSize = 1.0f;
	style.WindowRounding = 0.0f;

	ImGuiIO &io = ImGui::GetIO();
	//io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

	if (Engine::Renderer::GetCurrentAPI() == Engine::GraphicsAPI::OpenGL)
	{
		ImGui_ImplGlfw_InitForOpenGL(window, false);
		ImGui_ImplOpenGL3_Init();
	}
	else if (Engine::Renderer::GetCurrentAPI() == Engine::GraphicsAPI::Vulkan)
	{
		ImGui_ImplGlfw_InitForVulkan(window, false);
		Engine::VKRenderer *renderer = static_cast<Engine::VKRenderer*>(game->GetRenderer());
		Engine::VKBase &base = renderer->GetBase();
		ImGui_ImplVulkan_InitInfo info = {};
		info.Instance = base.GetInstance();
		info.PhysicalDevice = base.GetPhysicalDevice();
		info.Device = base.GetDevice();
		info.QueueFamily = base.GetGraphicsQueueFamily();
		info.Queue = base.GetGraphicsQueue();
		info.PipelineCache = VK_NULL_HANDLE;
		info.DescriptorPool = renderer->GetDescriptorPool();
		info.Allocator = nullptr;
		info.CheckVkResultFn = nullptr;
		ImGui_ImplVulkan_Init(renderer, &info, renderer->GetDefaultRenderPass());

		base.BeginSingleTimeCommands();
		ImGui_ImplVulkan_CreateFontsTexture(base.GetSingleTimeCommandbuffer());
		base.EndSingleTimeCommands();
		ImGui_ImplVulkan_InvalidateFontUploadObjects();
	}
	else if (Engine::Renderer::GetCurrentAPI() == Engine::GraphicsAPI::D3D11)
	{
		ImGui_ImplGlfw_InitForOpenGL(window, false);
		Engine::D3D11Renderer *renderer = static_cast<Engine::D3D11Renderer*>(game->GetRenderer());
		ImGui_ImplDX11_Init(renderer->GetDevice(), renderer->GetContext());
	}
	else
	{
		std::cout << "Only OpenGL and D3D11 are supported for Editor\n";
		return;
	}
	
	ImGui::LoadDock();

	this->game = game;

	game->GetEntityManager().AddComponentDestroyCallback(std::bind(&EditorNameManager::RemoveName, &editorNameManager, std::placeholders::_1));
	game->GetEntityManager().AddComponentDuplicateCallback(std::bind(&EditorNameManager::DuplicateEditorName, &editorNameManager, std::placeholders::_1, std::placeholders::_2));

	game->GetScriptManager().GetLuaState();

	luabridge::push(game->GetScriptManager().GetLuaState(), this);
	lua_setglobal(game->GetScriptManager().GetLuaState(), "Editor");

	camSpeed = game->GetEditorCamera().GetMoveSpeed();

	gizmo.Init(game);
	objectWindow.Init(game, this);
	sceneWindow.Init(game, this, &gizmo);
	terrainWindow.Init(game, this);
	materialWindow.Init(game, this);
	aiWindow.Init(game, this);
	renderingWindow.Init(game, this);
	animationWindow.Init(game, this);
	animationPropsWindow.Init(game, this);
	skeletonWindow.Init(game, this);
	consoleWindow.Init(game, this);
	assetsBrowserWindow.Init(game, this);

	FindFilesInDirectory("Data/Levels/*");

	// Add another render pass to which we will render the post process quad so it can be displayed as an image in the editor
	Engine::Pass &editorImguiPass = game->GetRenderingPath()->GetFrameGraph().AddPass("EditorImGUI");
	editorImguiPass.AddTextureInput("final");
	editorImguiPass.SetOnSetup([this](const Engine::Pass *thisPass)
	{ 
		if (Engine::Renderer::GetCurrentAPI() == Engine::GraphicsAPI::Vulkan)
		{
			ImGUI_ImplVulkan_SetGameViewTexture(this->game->GetRenderingPath()->GetFinalFBForEditor()->GetColorTexture());
		}
	});

	editorImguiPass.SetOnResized([this](const Engine::Pass *thisPass){});
	editorImguiPass.SetOnExecute([this]() { Render(); });

	game->GetRenderingPath()->GetFrameGraph().SetBackbufferSource("EditorImGUI");		// Post process pass writes to backbuffer
}

void EditorManager::Update(float dt)
{
	gizmo.Update(dt);
	gizmo.PrepareRender();

	if (Engine::Input::IsMousePressed(Engine::MouseButtonType::Left))
	{
		if (IsMouseInsideGameView())
			clickBeganOnGameView = true;
	}

	// Also check for intersection on click with the ui
	if (clickBeganOnGameView && Engine::Input::WasMouseButtonReleased(Engine::MouseButtonType::Left))
	{
		if (IsMouseInsideGameView() && game->GetRenderingPath()->IsUIEnabled())
		{
			glm::vec2 mousePos = Engine::Input::GetMousePosition();
			glm::vec2 flippedMousePos = mousePos;
			flippedMousePos.y = gameViewSize.y - flippedMousePos.y;

			//objectWindow.SetEntity(game->GetUIManager().PerformRaycast(flippedMousePos));		// Messes up the intersection with the 3d entites
		}
		clickBeganOnGameView = false;
	}

	if (game->HasSceneChanged())		// This works because EditorManager::Update() is called before Game::Update()
	{
		const std::vector<Engine::Scene> &scenes = game->GetScenes();

		Engine::Serializer s;
		s.OpenForReading(curLevelDir + "/" + scenes[game->GetCurrentSceneId()].name + ".names");
		if (s.IsOpen())
			editorNameManager.Deserialize(s);
		s.Close();

		gizmo.DeselectEntity();
		objectWindow.DeselectEntity();
	}

	if (gameViewSize.x != availableSize.x || gameViewSize.y != availableSize.y)		// To maybe reduced the resizes when dragging the dock
	{											
		// Check when the mouse is down and dragging, and only resize on mouse up
		if (availableSize.x > 0.0f && availableSize.y > 0.0f)
		{
			gameViewSize = availableSize;
			game->Resize((int)availableSize.x, (int)availableSize.y);

			if (Engine::Renderer::GetCurrentAPI() == Engine::GraphicsAPI::Vulkan)
			{
				ImGUI_ImplVulkan_SetGameViewTexture(game->GetRenderingPath()->GetFinalFBForEditor()->GetColorTexture(), true);
			}

			std::cout << "resize" << availableSize.x << "  " << availableSize.y <<  "\n";
		}
	}
}

void EditorManager::Render()
{
	// Needed so the gizmo ends up on the editor rt for the imgui texture
	/*game->GetRenderer()->SetRenderTarget(game->GetRenderingPath()->GetEditorRT());
	gizmo.Render();
	game->GetRenderer()->SetDefaultRenderTarget();*/

	// ImGui input
	ImGuiIO &io = ImGui::GetIO();
	for (int i = 0; i < 3; i++)
	{
		io.MouseDown[i] = mousePressed[i] || glfwGetMouseButton(window, i) != 0;    // If a mouse press event came, always pass it as "mouse held this frame", so we don't miss click-release events that are shorter than 1 frame.
		mousePressed[i] = false;
	}
	io.MouseWheel = mouseWheel;
	mouseWheel = 0.0f;

	if (Engine::Renderer::GetCurrentAPI() == Engine::GraphicsAPI::OpenGL)
		ImGui_ImplOpenGL3_NewFrame(game->GetRenderer());
	else if (Engine::Renderer::GetCurrentAPI() == Engine::GraphicsAPI::Vulkan)
		ImGui_ImplVulkan_NewFrame();
	else if (Engine::Renderer::GetCurrentAPI() == Engine::GraphicsAPI::D3D11)
		ImGui_ImplDX11_NewFrame(game->GetRenderer());
	
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();

	ShowMainMenuBar();

	ImVec2 displaySize = ImGui::GetIO().DisplaySize;

	ImVec2 toolBarSize = ImVec2(displaySize.x, 35.0f);
	ImVec2 toolBarPos = ImVec2(0.0f, (float)mainMenuHeight);

	ImVec2 statusBarSize = ImVec2(displaySize.x, 25.0f);
	ImVec2 statusBarPos = ImVec2(0.0f, displaySize.y - statusBarSize.y);

	ImVec2 docksPos = ImVec2(0.0f, mainMenuHeight + toolBarSize.y);
	ImVec2 docksSize = ImVec2(displaySize.x, displaySize.y - mainMenuHeight - toolBarSize.y - statusBarSize.y);

	// Draw tool bar
	ImGui::SetNextWindowPos(toolBarPos, ImGuiSetCond_Always);
	ImGui::SetNextWindowSize(toolBarSize, ImGuiSetCond_Always);
	ImGui::Begin("toolbar", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoResize);
	if (ImGui::Button("Play"))
	{
		game->Play();
		//objectWindow.DeselectEntity();		// Don't deselect so we can change in game the animation controller parameters
	}
	ImGui::SameLine();
	if (ImGui::Button("Pause"))
	{
		game->Pause();
	}
	ImGui::SameLine();
	if (ImGui::Button("Stop"))
	{
		game->Stop();
	}
	ImGui::SameLine();

	if (isProjectOpen)
	{
		const std::vector<Engine::Scene> &scenes = game->GetScenes();
		std::string str;
		for (size_t i = 0; i < scenes.size(); i++)
		{
			str += scenes[i].name + '\0';
		}
		if (ImGui::Combo("", &currentScene, str.c_str()))
		{
			game->SetScene(currentScene, std::string(projectName));

			Engine::Serializer s;
			s.OpenForReading(curLevelDir + "/" + scenes[currentScene].name + ".names");
			if (s.IsOpen())
				editorNameManager.Deserialize(s);
			s.Close();

			gizmo.DeselectEntity();							// Deselect on all three windows
			objectWindow.DeselectEntity();
		}

		ImGui::SameLine();
		if (ImGui::Button("New scene"))
			ImGui::OpenPopup("New Scene");

		if (ImGui::BeginPopup("New Scene"))
		{
			ImGui::InputText("Name", projectName, 256);
			if (ImGui::Button("OK"))
			{
				// Must be a valid name, not empty
				if (std::strcmp(projectName, "") != 0)
				{
					game->AddScene(std::string(projectName));
					ImGui::CloseCurrentPopup();
				}
			}
			ImGui::SameLine();
			if (ImGui::Button("Cancel"))
				ImGui::CloseCurrentPopup();

			ImGui::EndPopup();
		}
	}
	ImGui::End();

	// Draw status bar
	ImGui::SetNextWindowPos(statusBarPos, ImGuiSetCond_Always);
	ImGui::SetNextWindowSize(statusBarSize, ImGuiSetCond_Always);
	ImGui::Begin("statusbar", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoResize);
	//ImGui::Text("FPS: %f", ImGui::GetIO().Framerate);
	ImGui::Text("status");
	ImGui::End();

	if (needsProjectCreation)
	{
		HandleProjectCreation();
	}

	ImGui::RootDock(docksPos, docksSize);

	availableSize = HandleGameView();
	HandleSceneWindow();
	HandleObjectWindow();
	terrainWindow.Render();
	materialWindow.Render();
	aiWindow.Render();
	renderingWindow.Render();
	animationWindow.Render();
	animationPropsWindow.Render();
	skeletonWindow.Render();
	consoleWindow.Render();
	assetsBrowserWindow.Render();

	if (addingPrefab)
	{
		files.clear();
		Engine::utils::FindFilesInDirectory(files, curLevelDir + "/*", ".prefab");
		ImGui::OpenPopup("Choose Prefab");
		addingPrefab = false;
	}

	if (ImGui::BeginPopup("Choose Prefab"))
	{
		for (size_t i = 0; i < files.size(); i++)
		{
			if (ImGui::Selectable(files[i].c_str()))
			{			
				//game->GetSceneManager()->LoadPrefab(files[i]);
			}
		}

		ImGui::EndPopup();
	}

	if (needsSaving && firstLevelSave)
		SaveAs();

	if (needsLoading)
		DisplayLoadPopup();

	if (showExitPopup)
		ExitPopup();

	if (showUnsavedWarning)
		UnsavedWarning();

	ImGui::Render();

	if (Engine::Renderer::GetCurrentAPI() == Engine::GraphicsAPI::OpenGL)
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
	else if (Engine::Renderer::GetCurrentAPI() == Engine::GraphicsAPI::Vulkan)
		ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), static_cast<Engine::VKRenderer*>(game->GetRenderer())->GetCurrentCommamdBuffer());
	else if (Engine::Renderer::GetCurrentAPI() == Engine::GraphicsAPI::D3D11)
		ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

	if (Engine::Input::IsKeyPressed(KEY_LEFT_CONTROL) && Engine::Input::WasKeyReleased(KEY_S))
	{
		SaveProject();
	}
	if (Engine::Input::IsKeyPressed(KEY_LEFT_CONTROL) && Engine::Input::WasKeyReleased(KEY_Z) && !Engine::Input::IsKeyPressed(KEY_LEFT_SHIFT))
	{
		Undo();
	}
	if (Engine::Input::IsKeyPressed(KEY_LEFT_CONTROL) && Engine::Input::IsKeyPressed(KEY_LEFT_SHIFT) && Engine::Input::WasKeyReleased(KEY_Z))
	{
		Redo();
	}

	projectJustLoaded = false;
}

void EditorManager::Dispose()
{
	while (undoStack.empty() == false)
	{
		EditorCommand *cmd = undoStack.top();
		delete cmd;
		undoStack.pop();
	}
	while (redoStack.empty() == false)
	{
		EditorCommand *cmd = redoStack.top();
		delete cmd;
		redoStack.pop();
	}

	ImGui::SaveDock();
	ImGui::ShutdownDock();

	if (Engine::Renderer::GetCurrentAPI() == Engine::GraphicsAPI::OpenGL)
		ImGui_ImplOpenGL3_Shutdown();
	else if (Engine::Renderer::GetCurrentAPI() == Engine::GraphicsAPI::Vulkan)
		ImGui_ImplVulkan_Shutdown();
	else if (Engine::Renderer::GetCurrentAPI() == Engine::GraphicsAPI::D3D11)
		ImGui_ImplDX11_Shutdown();

	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();
}

void EditorManager::UpdateKeys(int key, int action, int mods)
{
	ImGuiIO& io = ImGui::GetIO();
	if (action == GLFW_PRESS)
		io.KeysDown[key] = true;
	if (action == GLFW_RELEASE)
		io.KeysDown[key] = false;

	(void)mods; // Modifiers are not reliable across systems
	io.KeyCtrl = io.KeysDown[GLFW_KEY_LEFT_CONTROL] || io.KeysDown[GLFW_KEY_RIGHT_CONTROL];
	io.KeyShift = io.KeysDown[GLFW_KEY_LEFT_SHIFT] || io.KeysDown[GLFW_KEY_RIGHT_SHIFT];
	io.KeyAlt = io.KeysDown[GLFW_KEY_LEFT_ALT] || io.KeysDown[GLFW_KEY_RIGHT_ALT];
	io.KeySuper = io.KeysDown[GLFW_KEY_LEFT_SUPER] || io.KeysDown[GLFW_KEY_RIGHT_SUPER];

	if (key == GLFW_KEY_ESCAPE && action == GLFW_RELEASE
		&& !needsLoading && !needsSaving && !needsProjectCreation
		&& game->GetState() == Engine::GameState::STOPPED)
	{
		showExitPopup = true;
	}
}

void EditorManager::UpdateScroll(double yoffset)
{
	mouseWheel += (float)yoffset; // Use fractional mouse wheel, 1.0 unit 5 lines.
	if (Engine::Input::IsMouseButtonDown(Engine::MouseButtonType::Right))
		game->GetEditorCamera().SetMoveSpeed(camSpeed += (mouseWheel * 2));
	if (camSpeed < 0.0f)
		camSpeed = 0.0f;
}

void EditorManager::UpdateMouse(int button, int action)
{
	if (action == GLFW_PRESS && button >= 0 && button < 3)
		mousePressed[button] = true;
}

void EditorManager::UpdateChar(unsigned int c)
{
	ImGuiIO& io = ImGui::GetIO();
	if (c > 0 && c < 0x10000)
		io.AddInputCharacter((unsigned short)c);
}

void EditorManager::OnFocus()
{
	// Only do this if the game is stopped
	if (game->GetState() == Engine::GameState::STOPPED)
	{
		// Update script properties when we gain focus of the window because we might add/remove some of them on the script file and by doing this we update it on the editor
		game->GetScriptManager().ReloadAll(); 
	}
}

void EditorManager::ShowMainMenuBar()
{
	ImGui::BeginMainMenuBar();
	if (ImGui::BeginMenu("File"))
	{
		if (ImGui::MenuItem("New", "CTRL+N"))
		{
			showUnsavedWarning = true;
		}
		if (ImGui::MenuItem("Open", "CTRL+O"))
		{
			needsLoading = true;
			FindFilesInDirectory("Data/Levels/*");
		}
		if (ImGui::MenuItem("Save", "CTRL+S"))
		{
			SaveProject();
		}
		ImGui::EndMenu();
	}
	if (ImGui::BeginMenu("Edit"))
	{
		bool canUndo = undoStack.size() > 0;
		bool canRedo = redoStack.size() > 0;

		if (ImGui::MenuItem("Undo", "CTRL+Z", nullptr, canUndo))
		{
			Undo();
		}
		if (ImGui::MenuItem("Redo", "CTRL+Y", false, canRedo))
		{
			Redo();
		}
		/*ImGui::Separator();
		if (ImGui::MenuItem("Copy", "CTRL+C"))
		{

		}
		if (ImGui::MenuItem("Paste", "CTRL+V"))
		{

		}*/
		ImGui::EndMenu();
	}
	if (ImGui::BeginMenu("Add"))
	{
		if (ImGui::MenuItem("Entity"))
		{
			Engine::Entity e = game->AddEntity();
			char name[128];
			sprintf(name, "%u", e.id);
			editorNameManager.SetName(e, name);
			objectWindow.SetEntity(e);
			gizmo.SetSelectedEntity(e);
		}
		/*if (ImGui::MenuItem("AI Object"))
		{
			//gizmo.SetSelectedObject(game->AddAIObject());
			//objectWindow.SetObject(gizmo.GetSelectedObject());
		}*/
		if (ImGui::MenuItem("Entity Prefab"))
		{
			addingPrefab = true;
		}
		if (ImGui::MenuItem("Terrain"))
		{
			Engine::TerrainInfo info = {};
			info.matPath = "Data/Resources/Materials/terrain/default_terrain.mat";
			game->AddTerrain(info);
		}
		if (ImGui::BeginMenu("Material"))
		{
			if (ImGui::MenuItem("Default"))
			{
				Engine::VertexAttribute attribs[4] = {};
				attribs[0].count = 3;						// Position
				attribs[1].count = 2;						// UV
				attribs[2].count = 3;						// Normal
				attribs[3].count = 3;						// Color

				attribs[0].offset = 0;
				attribs[1].offset = 3 * sizeof(float);
				attribs[2].offset = 5 * sizeof(float);
				attribs[3].offset = 8 * sizeof(float);

				Engine::VertexInputDesc desc = {};
				desc.stride = sizeof(Engine::VertexPOS3D_UV_NORMAL_COLOR);
				desc.attribs = { attribs[0], attribs[1], attribs[2], attribs[3] };

				Engine::MaterialInstance *m = game->GetRenderer()->CreateMaterialInstanceFromBaseMat(game->GetScriptManager(), "Data/Resources/Materials/default_mat.lua", { desc });
				materialWindow.SetCurrentMaterialInstance(m);
			}
			if (ImGui::MenuItem("PBR"))
			{

			}
			ImGui::EndMenu();
		}
		ImGui::EndMenu();
	}
	if (ImGui::BeginMenu("View"))
	{
		bool showObjectWindow = objectWindow.IsVisible();
		bool showSceneWindow = sceneWindow.IsVisible();
		bool showTerrainWindow = terrainWindow.IsVisible();
		bool showMaterialWindow = materialWindow.IsVisible();
		bool showAIWindow = aiWindow.IsVisible();
		bool showRenderingWindow = renderingWindow.IsVisible();
		bool showAnimationWindow = animationWindow.IsVisible();
		bool showAnimationPropsWindow = animationPropsWindow.IsVisible();
		bool showSkeletonTreeWindow = skeletonWindow.IsVisible();
		bool showConsoleWindow = consoleWindow.IsVisible();
		bool showAssetsBrowserWindow = assetsBrowserWindow.IsVisible();

		ImGui::MenuItem("Game View", nullptr, &showGameView);

		if (ImGui::MenuItem("Object Window", nullptr, &showObjectWindow))
		{
			objectWindow.Show(showObjectWindow);
		}
		if (ImGui::MenuItem("Scene Window", nullptr, &showSceneWindow))
		{
			sceneWindow.Show(showSceneWindow);
		}
		if (ImGui::MenuItem("Terrain Window", nullptr, &showTerrainWindow))
		{
			terrainWindow.Show(showTerrainWindow);
		}
		if (ImGui::MenuItem("Material Window", nullptr, &showMaterialWindow))
		{
			materialWindow.Show(showMaterialWindow);
		}
		if (ImGui::MenuItem("AI Window", nullptr, &showAIWindow))
		{
			aiWindow.Show(showAIWindow);
		}
		if (ImGui::MenuItem("Rendering Window", nullptr, &showRenderingWindow))
		{
			renderingWindow.Show(showRenderingWindow);
		}
		if (ImGui::MenuItem("Animation Window", nullptr, &showAnimationWindow))
		{
			animationWindow.Show(showAnimationWindow);
		}
		if (ImGui::MenuItem("Animation Properties Window", nullptr, &showAnimationPropsWindow))
		{
			animationPropsWindow.Show(showAnimationPropsWindow);
		}
		if (ImGui::MenuItem("Skeleton Tree Window", nullptr, &showSkeletonTreeWindow))
		{
			skeletonWindow.Show(showSkeletonTreeWindow);
		}
		if (ImGui::MenuItem("Console Window", nullptr, &showConsoleWindow))
		{
			consoleWindow.Show(showConsoleWindow);
		}
		if (ImGui::MenuItem("Assets Browser", nullptr, &showAssetsBrowserWindow))
		{
			assetsBrowserWindow.Show(showAssetsBrowserWindow);
		}

		ImGui::EndMenu();
	}

	mainMenuHeight = (int)ImGui::GetWindowSize().y;

	ImGui::EndMainMenuBar();
}

void EditorManager::ShowGameViewToolBar()
{
	if (ImGui::DragFloat("Cam speed", &camSpeed, 0.1f))
	{
		game->GetEditorCamera().SetMoveSpeed(camSpeed);
	}
}

ImVec2 EditorManager::HandleGameView()
{
	ImVec2 availableSize;

	if (ImGui::BeginDock("Game View", &showGameView))
	{
		availableSize = ImGui::GetContentRegionAvail();
		availableSize.y -= 18.0f;

		if (Engine::Renderer::GetCurrentAPI() == Engine::GraphicsAPI::D3D11 || Engine::Renderer::GetCurrentAPI() == Engine::GraphicsAPI::Vulkan)
			ImGui::Image((ImTextureID)(game->GetRenderingPath()->GetFinalFBForEditor()->GetColorTexture()), availableSize, ImVec2(0, 0), ImVec2(1, 1));
		else
			ImGui::Image((ImTextureID)(game->GetRenderingPath()->GetFinalFBForEditor()->GetColorTexture()), availableSize, ImVec2(0, 1), ImVec2(1, 0));

		glm::vec2 mousePos = Engine::Input::GetMousePosition();

		ImVec2 minSize = ImGui::GetItemRectMin();
		viewportPos.x = minSize.x;
		viewportPos.y = minSize.y;
		gizmo.SetRayPoint(mousePos);

		// Don't let the raycast to select and object be made if the mouse if not in the game view otherwise it would unselect and object when we could be trying to modify some value
		// Don't let the raycast when the save level popup is open
		// Dont' let the gizmo raycast when the create/open project popup is open
		if (!needsSaving && !needsLoading && !needsProjectCreation)
		{
			if (mousePos.x > 0.0f && mousePos.x < availableSize.x && mousePos.y > 0.0f && mousePos.y < availableSize.y)
			{
				if (objectWindow.IsSelectingAsset() == false)
					gizmo.CanRaycast(true);
			}
			else
				gizmo.CanRaycast(false);
		}

		ShowGameViewToolBar();
	}
	ImGui::EndDock();

	//Log::Vec2(viewportPos);

	if (terrainWindow.IsSelected() && ImGui::IsMouseClicked(0))
	{
		glm::vec2 mousePos = Engine::Input::GetMousePosition();
		if (mousePos.x > 0.0f && mousePos.x < availableSize.x && mousePos.y > 0.0f && mousePos.y < availableSize.y)
		{
			terrainWindow.Paint(mousePos);
		}
	}

	return availableSize;
}

void EditorManager::HandleObjectWindow()
{
	if (gizmo.GetSelectedEntity().IsValid() && gizmo.WasEntitySelected())
	{
		objectWindow.SetEntity(gizmo.GetSelectedEntity());
	}
	objectWindow.Render();
}

void EditorManager::HandleSceneWindow()
{
	sceneWindow.Render();
}

void EditorManager::HandleProjectCreation()
{
	ImGui::OpenPopup("Project");
	if (ImGui::BeginPopupModal("Project", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
	{
		ImGui::Text("Create a project...");
		ImGui::InputText("", projectName, 256);

		if (ImGui::Button("Create Project"))
		{
			// Must be a valid name, not empty
			if (std::strcmp(projectName, "") != 0)
			{
				bool canCreateFolder = true;

				// Don't allow project creation if we already have a project folder with the same name
				for (size_t i = 0; i < folderInLevelFolder.size(); i++)
				{
					if (projectName == folderInLevelFolder[i])
					{
						canCreateFolder = false;
						ImGui::OpenPopup("Project already exists!");
						break;
					}
				}

				if (canCreateFolder)
				{
					curLevelDir = "Data/Levels/";
					
					if (!DirectoryExists(curLevelDir))
						CreateFolder(curLevelDir.c_str());

					curLevelDir += projectName;

					CreateFolder(curLevelDir.c_str());

					game->AddScene("main");		// Add default main scene
					game->Save(curLevelDir + '/', std::string(projectName));			

					firstLevelSave = false;
					needsProjectCreation = false;
					isProjectOpen = true;
				}
			}
		}

		if (ImGui::BeginPopupModal("Project already exists!"))
		{
			if (ImGui::Button("OK", ImVec2(200, 0)))
			{
				ImGui::CloseCurrentPopup();
			}
			ImGui::EndPopup();
		}

		ImGui::Text("... or open one");

		for (size_t i = 0; i < folderInLevelFolder.size(); i++)
		{
			if (ImGui::Selectable(folderInLevelFolder[i].c_str()))
			{
				std::strcpy(projectName, folderInLevelFolder[i].c_str());		// Make the current level name equal to the current folder name

				std::string projectNameStr = folderInLevelFolder[i];
				curLevelDir = "Data/Levels/" + projectNameStr;

				std::chrono::high_resolution_clock::time_point t1 = std::chrono::high_resolution_clock::now();
				game->Load(folderInLevelFolder[i]);
				std::chrono::high_resolution_clock::time_point t2 = std::chrono::high_resolution_clock::now();

				auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count();
				std::cout << "Level load time: " << duration << " ms\n";

				assetsBrowserWindow.SetFiles(curLevelDir);

				currentScene = game->GetCurrentSceneId();

				const std::vector<Engine::Scene> &scenes = game->GetScenes();
				if (scenes.size() > 0)
				{
					const std::string &name = game->GetScenes()[currentScene].name;

					Engine::Serializer s;
					s.OpenForReading(curLevelDir + "/" + name + ".names");
					if (s.IsOpen())
						editorNameManager.Deserialize(s);
					s.Close();

					game->GetRenderingPath()->LoadRenderingSettings(curLevelDir + '/' + name + ".rendersettings");
				}

				aiWindow.Update();

				needsLoading = false;
				firstLevelSave = false;
				needsProjectCreation = false;
				isProjectOpen = true;
				projectJustLoaded = true;
			}
		}

		ImGui::EndPopup();
	}
}

void EditorManager::DisplayLoadPopup()
{
	ImGui::OpenPopup("Choose level");
	if (ImGui::BeginPopupModal("Choose level", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove))
	{
		for (size_t i = 0; i < folderInLevelFolder.size(); i++)
		{
			if (ImGui::Selectable(folderInLevelFolder[i].c_str()))
			{
				std::strcpy(projectName, folderInLevelFolder[i].c_str());

				std::string projectNameStr = folderInLevelFolder[i];
				curLevelDir = "Data/Levels/" + projectNameStr;

				game->Load(folderInLevelFolder[i]);
				currentScene = game->GetCurrentSceneId();

				const std::string &name = game->GetScenes()[currentScene].name;

				Engine::Serializer s;
				s.OpenForReading(curLevelDir + "/" + name + ".names");
				if (s.IsOpen())
					editorNameManager.Deserialize(s);
				s.Close();

				game->GetRenderingPath()->LoadRenderingSettings(curLevelDir + '/' + name + ".rendersettings");

				needsLoading = false;
				firstLevelSave = false;
				isProjectOpen = true;
				projectJustLoaded = true;
			}
		}

		if (ImGui::Button("Cancel", ImVec2(100, 0)))
		{
			needsLoading = false;
		}
		ImGui::EndPopup();
	}
}

void EditorManager::SaveAs()
{
	ImGui::OpenPopup("Level name");
	if (ImGui::BeginPopupModal("Level name", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove))
	{
		ImGui::InputText("", projectName, 256);
		if (ImGui::Button("Save"))
		{
			std::string projectNameStr = std::string(projectName);
			std::string path = "Data/Levels/" + projectNameStr + '/';
			if (game->Save(path, projectNameStr))
				std::cout << "Project saved\n";
			else
				std::cout << "Failed to save project\n";

			const std::string &name = game->GetScenes()[currentScene].name;

			Engine::Serializer s;
			s.OpenForWriting();
			editorNameManager.Serialize(s);
			s.Save(curLevelDir + "/" + name + ".names");
			s.Close();

			ImGui::CloseCurrentPopup();

			needsSaving = false;
			firstLevelSave = false;
		}

		ImGui::SameLine();
		if (ImGui::Button("Cancel"))
		{
			needsSaving = false;
		}
		ImGui::EndPopup();
	}
}

void EditorManager::ExitPopup()
{
	ImGui::OpenPopup("Are you sure you want to exit?");
	if (ImGui::BeginPopupModal("Are you sure you want to exit?", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
	{
		if (ImGui::Button("OK", ImVec2(100, 0)))
		{
			glfwSetWindowShouldClose(window, true);

			ImGui::CloseCurrentPopup();
			showExitPopup = false;
		}
		ImGui::SameLine();
		if (ImGui::Button("Cancel", ImVec2(100, 0)))
		{
			ImGui::CloseCurrentPopup();
			showExitPopup = false;
		}
		ImGui::EndPopup();
	}
}

void EditorManager::UnsavedWarning()
{
	ImGui::OpenPopup("Unsaved changes will be lost! Continue?");
	if (ImGui::BeginPopupModal("Unsaved changes will be lost! Continue?", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
	{
		if (ImGui::Button("Yes", ImVec2(170, 0)))
		{
			game->PartialDispose();

			ImGui::CloseCurrentPopup();
			showUnsavedWarning = false;
		}
		ImGui::SameLine();
		if (ImGui::Button("No", ImVec2(170, 0)))
		{
			ImGui::CloseCurrentPopup();
			showUnsavedWarning = false;
		}
		ImGui::EndPopup();
	}
}

void EditorManager::FindFilesInDirectory(const char *dir)
{
	folderInLevelFolder.clear();

	WIN32_FIND_DATAA findData;
	HANDLE h = FindFirstFileA(dir, &findData);		// Use FindFirstFileA to use the ANSI version so we don't need to cast to LPCWSTR which also doesn't work

	if (INVALID_HANDLE_VALUE == h)
	{
		std::cout << "Failed to get files in directory!\n";
		return;
	}

	do
	{
		if (findData.cFileName[0] != '.' && findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		{
			folderInLevelFolder.push_back(std::string(findData.cFileName));
		}

	} while (FindNextFileA(h, &findData));

	FindClose(h);
}

bool EditorManager::DirectoryExists(const std::string &path)
{
	return std::experimental::filesystem::exists(path);
}

bool EditorManager::CreateFolder(const char *folderPath)
{
	Engine::Log::Print(Engine::LogLevel::LEVEL_INFO, "Creating folder at %s ...", folderPath);

	if (CreateDirectoryA(folderPath, NULL))
	{
		Engine::Log::Print(Engine::LogLevel::LEVEL_INFO, "Done");
		return true;
	}

	Engine::Log::Print(Engine::LogLevel::LEVEL_ERROR, "Failed to create folder: %s    Error: %lu", folderPath, GetLastError());

	return false;
}

void EditorManager::SaveProject()
{
	if (!firstLevelSave)
	{
		std::string projectNameStr = std::string(projectName);
		std::string path = "Data/Levels/" + projectNameStr + '/';
		if (game->Save(path, projectNameStr))
			std::cout << "Project saved\n";
		else
			std::cout << "Failed to save project\n";

		const std::string &name = game->GetScenes()[currentScene].name;

		Engine::Serializer s;
		s.OpenForWriting();
		editorNameManager.Serialize(s);
		s.Save(curLevelDir + "/" + name + ".names");
		s.Close();

		game->GetRenderingPath()->SaveRenderingSettings(curLevelDir + '/' + name + ".rendersettings");
	}
	else
		needsSaving = true;
}

void EditorManager::Undo()
{
	if (undoStack.size() > 0)
	{
		EditorCommand *cmd = undoStack.top();
		cmd->Undo();
		undoStack.pop();

		redoStack.push(cmd);
	}
}

void EditorManager::Redo()
{
	if (redoStack.size() > 0)
	{
		EditorCommand *cmd = redoStack.top();
		cmd->Redo();
		redoStack.pop();

		undoStack.push(cmd);
	}
}

bool EditorManager::IsMouseInsideGameView() const
{
	glm::vec2 mousePos = Engine::Input::GetMousePosition();
	if (mousePos.x > 0.0f && mousePos.x < gameViewSize.x && mousePos.y > 0.0f && mousePos.y < gameViewSize.y)
		return true;

	return false;
}

void EditorManager::AddEntityScriptProperty(const std::string &propName, Engine::Entity eWithProp)
{
	if (!eWithProp.IsValid())
		return;

	game->GetScriptManager().GetScript(eWithProp)->AddProperty(propName);
}