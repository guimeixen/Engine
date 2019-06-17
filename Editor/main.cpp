//#include <vld.h>

#include "Application.h"
#include "Graphics/MeshDefaults.h"

class MyApplication : public Engine::Application
{
public:
	bool Init(Engine::GraphicsAPI api, unsigned int width, unsigned int height) override
	{
		if (!Engine::Application::Init(api, width, height))
			return false;

		float vertices[] = {
			-0.5f, -0.5f, 0.0f, 0.0f,
			0.5f, -0.5f, 1.0f, 0.0f,
			-0.5f, 0.5f, 0.0f, 1.0f,
			0.5f, 0.5f, 1.0f, 1.0f
		};

		unsigned short indices[] = { 0,1,2, 2,1,3 };

		Engine::VertexAttribute pos = {};
		pos.count = 2;
		pos.offset = 0;
		pos.vertexAttribFormat = Engine::VertexAttributeFormat::FLOAT;

		Engine::VertexAttribute uv = {};
		uv.count = 2;
		uv.offset = 2 * sizeof(float);
		uv.vertexAttribFormat = Engine::VertexAttributeFormat::FLOAT;

		Engine::VertexInputDesc desc = {};
		desc.attribs.push_back(pos);
		desc.attribs.push_back(uv);
		desc.stride = 4 * sizeof(float);

		Engine::Entity e = game.AddEntity();

		triangle = Engine::MeshDefaults::CreateFromVertices(renderer, desc, 4, vertices, sizeof(vertices), 6, indices, sizeof(indices));

		Engine::MaterialInstance *mat = renderer->CreateMaterialInstanceFromBaseMat(game.GetScriptManager(), "Data/Resources/Materials/default_mat.lua", { desc });

		Engine::Model *model = game.GetModelManager().AddModelFromMesh(e, triangle, mat, {});

		Engine::Camera *cam = game.GetMainCamera();
		cam->SetPosition(glm::vec3(0.0f, 0.0f, -3.0f));
		cam->SetFrontAndUp(glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f, 1.0f, 0.0f));

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

private:
	Engine::Mesh triangle;
};

int main()
{
	const unsigned int WIDTH = 1280;
	const unsigned int HEIGHT = 720;

	MyApplication app;
	if (!app.Init(Engine::GraphicsAPI::OpenGL, WIDTH, HEIGHT))
		return 1;

	return app.Run();
}