#include "Application.h"

#include "Program\Utils.h"
#include "Graphics\ResourcesLoader.h"
#include "Graphics\Effects\MainView.h"

#include <chrono>
#include <iostream>

namespace Engine
{
	bool Application::Init(GraphicsAPI api, unsigned int width, unsigned int height)
	{
		if (!window.Init(api, width, height))
			return false;

		renderer = Renderer::Create(window.GetHandle(), api, width, height, window.GetMonitorWidth(), window.GetMonitorHeight());

		game.Init(renderer);

#ifdef EDITOR
		editorManager.Init(window.GetHandle(), &game);
		window.SetEditorManager(&editorManager);
		game.GetRenderingPath()->GetFrameGraph().Bake(renderer);
#endif

		/*game.GetRenderingPath()->GetFrameGraph().Bake(renderer);
		game.GetRenderingPath()->GetFrameGraph().Setup();*/

		game.GetRenderingPath()->GetFrameGraph().Setup();

		renderer->PostLoad();

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
		Engine::ResourcesLoader::Clean();
		
#ifdef EDITOR	
		editorManager.Dispose();
#endif

		if (renderer)
			delete renderer;

		window.Dispose();
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
			
			Update();

			if (!window.IsMinimized())
				Render();

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
