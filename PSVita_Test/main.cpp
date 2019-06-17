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
		
		float vertices[] = {
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
		desc.stride = 4 * sizeof(float);
		
		Engine::Entity e = game.AddEntity();
		game.GetScriptManager().AddScript(e, "Data/test.lua");
		
		triangle = Engine::MeshDefaults::CreateFromVertices(renderer, desc, 4, vertices, sizeof(vertices), 6, indices, sizeof(indices));

		Engine::MaterialInstance *mat = renderer->CreateMaterialInstance(game.GetScriptManager(), "Data/Materials/sand.mat", { desc });	

		if (mat->textures[0] == nullptr)
		{
			Engine::TextureParams params = {};
			params.format = Engine::TextureFormat::RGBA;
			mat->textures[0] = renderer->CreateTexture2D("Data/Textures/sand.png", params);
		}

		Engine::Model *model = game.GetModelManager().AddModelFromMesh(e, triangle, mat, {});
		
		//game.GetModelManager().AddModel(e, "Data/Models/trash_can.oj");

		return true;
	}

	void Update()
	{
		PSVitaApplication::Update();
	}

	void Render()
	{
		//Engine::RenderItem ri = {};
		//ri.mesh = triange;
		//renderer->Submit(ri);
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