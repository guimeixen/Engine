#pragma once

#include "Graphics/Renderer.h"
#include "Game/Game.h"
#include "Program/Window.h"
#include "Program/FileManager.h"

namespace Engine
{
	class Application
	{
	public:
		virtual bool Init(GraphicsAPI api, unsigned int width, unsigned int height);
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

		float deltaTime = 0.0f;
		float lastTime = 0.0f;

	private:
		Window window;
		int frameCounter = 0;
		float lastFPS = 0.0f;
		float fpsTimer = 0.0f;
		float total = 0.0f;
		int count = 0;

#ifdef EDITOR
		EditorManager editorManager;
#endif
	};
}
