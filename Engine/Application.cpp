#include "Application.h"

#include "Program/Utils.h"
#include "Graphics/ResourcesLoader.h"
#include "Program/Random.h"
#include "Program/Log.h"

#include <chrono>
#include <iostream>

namespace Engine
{
	bool Application::Init(GraphicsAPI api, unsigned int width, unsigned int height)
	{
		if (!window.Init(&inputManager, api, width, height))
			return false;

		fileManager.Init();

		Random::Init();

		renderer = Renderer::Create(window.GetHandle(), api, &fileManager, width, height, window.GetMonitorWidth(), window.GetMonitorHeight());

		if (renderer == nullptr)
		{
			Log::Print(LogLevel::LEVEL_ERROR, "Failed to create renderer!");
			return false;
		}
		game.Init(&allocator, renderer, &fileManager, &inputManager);

#ifdef EDITOR
		editorManager.Init(window.GetHandle(), &game, &inputManager);
		window.SetEditorManager(&editorManager);
		game.GetRenderingPath()->GetFrameGraph().Bake(renderer);
#endif

		/*game.GetRenderingPath()->GetFrameGraph().Bake(renderer);
		game.GetRenderingPath()->GetFrameGraph().Setup();*/

		if (!renderer->PostLoad(game.GetScriptManager()))
			return false;

		game.GetRenderingPath()->GetFrameGraph().Setup();

		allocator.PrintStats();

		return true;
	}

	void Application::Update()
	{
#ifdef EDITOR
		editorManager.Update(deltaTime);
#endif

		
#ifndef EDITOR
		if (window.WasResized())
		{
			renderer->Resize(window.GetWidth(), window.GetHeight());		// Always update the swap chain when the window is resized
			game.Resize(window.GetWidth(), window.GetHeight());
		}
#else
		// Don't call Resize of the game when in Editor because it will be automatically called by the editor manager when detecting that the game view has a different size
		if (window.WasResized())
		{
			renderer->Resize(window.GetWidth(), window.GetHeight());
		}
#endif	
		game.Update(deltaTime);
	}

	void Application::Render()
	{
		renderer->BeginFrame();
		game.Render(renderer);

		if (Renderer::GetCurrentAPI() == GraphicsAPI::OpenGL)
			glfwSwapBuffers(window.GetHandle());
		else
			renderer->Present();
	}

	void Application::Dispose()
	{
		renderer->WaitIdle();
		game.Dispose();
		ResourcesLoader::Clean();
		
#ifdef EDITOR	
		editorManager.Dispose();
#endif

		if (renderer)
			delete renderer;

		window.Dispose();

		Log::Close();
	}

	int Application::Run()
	{
		while (!window.ShouldClose())
		{
			window.UpdateInput();

			auto tStart = std::chrono::high_resolution_clock::now();


#ifndef EDITOR					// Only allow the game to be shutdown through script if we're not in the Editor
			if (game.ShouldShutdown())
				glfwSetWindowShouldClose(window.GetHandle(), true);
#endif

			double currentTime = glfwGetTime();
			deltaTime = (float)currentTime - lastTime;
			lastTime = (float)currentTime;
			
			if (!window.IsMinimized())
			{
				Update();
				Render();
			}

			frameCounter++;
			auto tEnd = std::chrono::high_resolution_clock::now();
			auto tDiff = std::chrono::duration<double, std::milli>(tEnd - tStart).count();
			fpsTimer += (float)tDiff;

			if (fpsTimer > 1000.0f)
			{
				lastFPS = (float)frameCounter * (1000.0f / fpsTimer);
				renderer->SetFrameTime(1000.0f / lastFPS);
				count++;
				//std::cout << "fps:" << lastFPS << '\n';
				total += 1000.0f / lastFPS;
				//std::cout << "ms:" << total / count << '\n';
				fpsTimer = 0.0f;
				frameCounter = 0;		
			}
		}

		Dispose();

		return 0;
	}
}
