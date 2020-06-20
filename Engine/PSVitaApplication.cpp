#include "PSVitaApplication.h"

#include "Program/Random.h"
#include "Program/Log.h"
#include "Engine/Graphics/ResourcesLoader.h"

#include "psp2/display.h"
#include "psp2/ctrl.h"
#include "psp2/kernel/processmgr.h"
#include "psp2/kernel/sysmem.h"

#include <chrono>

namespace Engine
{
	bool PSVitaApplication::Init()
	{
		Log::Print(LogLevel::LEVEL_INFO, "Engine startup\n");

		SceKernelFreeMemorySizeInfo info = {};
		info.size = sizeof(SceKernelFreeMemorySizeInfo);
		sceKernelGetFreeMemorySize(&info);
		Log::Print(LogLevel::LEVEL_INFO, "USER_CDRAM_RW memory available size: %d\n", info.size_cdram);
		Log::Print(LogLevel::LEVEL_INFO, "USER_MAIN_PHYCONT_*_RW available size: %d\n", info.size_phycont);
		Log::Print(LogLevel::LEVEL_INFO, "USER_RW available size: %d\n\n", info.size_user);

		fileManager.Init();
		inputManager.LoadInputMappings(&fileManager, "");

		Random::Init();

		renderer = Renderer::Create(nullptr, GraphicsAPI::GXM, &fileManager, 960, 544, 960, 544);

		sceKernelGetFreeMemorySize(&info);
		Log::Print(LogLevel::LEVEL_INFO, "USER_CDRAM_RW memory available size: %d\n", info.size_cdram);
		Log::Print(LogLevel::LEVEL_INFO, "USER_MAIN_PHYCONT_*_RW available size: %d\n", info.size_phycont);
		Log::Print(LogLevel::LEVEL_INFO, "USER_RW available size: %d\n\n", info.size_user);

		if (!renderer)
			return false;

		game.Init(&allocator, renderer, &fileManager);
		///game.GetRenderingPath()->GetFrameGraph().Setup();

		sceKernelGetFreeMemorySize(&info);
		Log::Print(LogLevel::LEVEL_INFO, "USER_CDRAM_RW memory available size: %d\n", info.size_cdram);
		Log::Print(LogLevel::LEVEL_INFO, "USER_MAIN_PHYCONT_*_RW available size: %d\n", info.size_phycont);
		Log::Print(LogLevel::LEVEL_INFO, "USER_RW available size: %d\n\n", info.size_user);

		renderer->PostLoad();

		allocator.PrintStats();

		// Enable analog stick
		sceCtrlSetSamplingMode(SCE_CTRL_MODE_ANALOG);

		return true;
	}

	void PSVitaApplication::Update()
	{
		// I don't think it's needed, because the app will not resize on the Vita
		/*if (window.WasResized())
		{
			renderer->Resize(window.GetWidth(), window.GetHeight());		// Always update the swap chain when the window is resized
			game.Resize(window.GetWidth(), window.GetHeight());
		}*/

		game.Update(deltaTime);
	}

	void PSVitaApplication::Render()
	{
		renderer->BeginFrame();
		game.Render(renderer);
		renderer->Present();
	}

	void PSVitaApplication::Dispose()
	{
		renderer->WaitIdle();
		game.Dispose();
		Engine::ResourcesLoader::Clean();

		if (renderer)
			delete renderer;

		Log::Close();
	}

	int PSVitaApplication::Run()
	{
		SceCtrlData ctrlData = {};
		bool quit = false;

		while (!quit)
		{
			//window.UpdateInput();
			sceCtrlPeekBufferPositive(0, &ctrlData, 1);

			inputManager.UpdateVitaButtons(ctrlData.buttons);
			inputManager.UpdateVitaSticks(ctrlData.lx, ctrlData.ly, ctrlData.rx, ctrlData.ry);

			if (ctrlData.buttons & SCE_CTRL_START)
			{
				Log::Print(LogLevel::LEVEL_INFO, "Stopping\n");
				break;
			}

			auto tStart = std::chrono::high_resolution_clock::now();


			// Only allow the game to be shutdown through script if we're not in the Editor
			// Don't allow shutdown if we're in the Vita
			///if (game.ShouldShutdown())
			///	break;

			double currentTime = (double)sceKernelGetProcessTimeWide();
			currentTime /= 1000000.0;
			deltaTime = (float)currentTime - lastTime;
			lastTime = (float)currentTime;

			Update();
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