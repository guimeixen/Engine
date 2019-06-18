//#include <vld.h>

#include "PSVitaApplication.h"
#include "Program/Log.h"
#include "Graphics/MeshDefaults.h"
#include "Graphics/VertexArray.h"

class MyApplication : public Engine::PSVitaApplication
{
public:
	bool Init()
	{
		if (!PSVitaApplication::Init())
			return false;
		
		/*float vertices[] = {
			-0.5f, -0.5f, 0.0f, 0.0f,
			0.5f, -0.5f, 1.0f, 0.0f,
			-0.5f, 0.5f, 0.0f, 1.0f,
			0.5f, 0.5f, 1.0f, 1.0f
		};

		unsigned short indices[] = {0,1,2, 2,1,3};

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
		desc.stride = 4 * sizeof(float);*/
		
		Engine::Entity e = game.AddEntity();
		//game.GetScriptManager().AddScript(e, "Data/test.lua");
		
		//triangle = Engine::MeshDefaults::CreateFromVertices(renderer, desc, 4, vertices, sizeof(vertices), 6, indices, sizeof(indices));

		/*Engine::MaterialInstance *mat = renderer->CreateMaterialInstance(game.GetScriptManager(), "Data/Materials/sand.mat", { desc });*/

		game.GetModelManager().AddModel(e, "Data/Models/trash_can.obj");
		
		//Engine::Model *model = game.GetModelManager().AddModelFromMesh(e, triangle, mat, {});
		
		Engine::FPSCamera *fpsCamera = static_cast<Engine::FPSCamera*>(game.GetMainCamera());
		fpsCamera->SetProjectionMatrix(70.0f, renderer->GetWidth(), renderer->GetHeight(), 0.2f, 700.0f);
		fpsCamera->SetPosition(glm::vec3(0.0f, 0.0f, 1.0f));
		fpsCamera->SetPitch(0.0f);
		fpsCamera->SetYaw(180.0f);
		return true;
	}

	void Update()
	{
		PSVitaApplication::Update();
	}

	void Render()
	{
		PSVitaApplication::Render();
	}

	/*void Dispose()
	{
		PSVitaApplication::Dispose();
	}*/

private:
	Engine::Mesh triangle;
};

int main()
{
	MyApplication app;
	if (!app.Init())
		return 1;

	return app.Run();
}