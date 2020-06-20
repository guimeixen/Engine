#pragma once

#include "Graphics/Renderer.h"
#include "Game/Game.h"
#include "Program/Input.h"
#include "Program/FileManager.h"
#include "Program/Allocator.h"

namespace Engine
{
	class PSVitaApplication
	{
	public:
		virtual bool Init();
		virtual void Update();
		virtual void Render();
		virtual void Dispose();

		int Run();

		Game &GetGame() { return game; }
		Renderer *GetRenderer() const { return renderer; }

	protected:
		Game game;
		Renderer *renderer;
		InputManager inputManager;
		FileManager fileManager;
		Allocator allocator;

		float deltaTime = 0.0f;
		float lastTime = 0.0f;

	private:
		int frameCounter = 0;
		float lastFPS = 0.0f;
		float fpsTimer = 0.0f;
		float total = 0.0f;
		int count = 0;
	};
}
