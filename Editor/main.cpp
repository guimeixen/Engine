//#include <vld.h>

#include "Application.h"

class MyApplication : public Engine::Application
{
public:
	bool Init(Engine::GraphicsAPI api, unsigned int width, unsigned int height) override
	{
		if (!Engine::Application::Init(api, width, height))
			return false;

		return true;
	}
	void Update() override
	{
		Application::Update();
	}
	void Render() override
	{
		Application::Render();
	}
};

int main()
{
	const unsigned int WIDTH = 1280;
	const unsigned int HEIGHT = 720;

	MyApplication app;
	if (!app.Init(Engine::GraphicsAPI::Vulkan, WIDTH, HEIGHT))
		return 1;

	return app.Run();
}