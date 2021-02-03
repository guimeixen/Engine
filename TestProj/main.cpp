//#include <vld.h>

#include "Application.h"
#include "Graphics/MeshDefaults.h"
#include "Program/Random.h"
#include "Graphics/Effects/ForwardRenderer.h"

#include "include/glm/gtc/matrix_transform.hpp"

#include <chrono>

class MyApplication : public Engine::Application
{
public:
	bool Init(Engine::GraphicsAPI api, unsigned int width, unsigned int height)
	{
		if (!Engine::Application::Init(api, width, height))
			return false;

		/*Engine::ForwardRenderer *forwardRenderer = new Engine::ForwardRenderer();
		game.SetRendererConfig(forwardRenderer);

		forwardRenderer.EnableVolumetricClouds(false);
		forwardRenderer.EnableOcean(false);
		forwardRenderer.EnableBloom(false);*/

		//game.LoadProject("models");

		auto time = std::chrono::system_clock::now();
		time_t tt = std::chrono::system_clock::to_time_t(time);

		//tm utc_tm = *gmtime(&tt);
		tm local_tm = *localtime(&tt);

		std::cout << local_tm.tm_year + 1900 << '\n';
		std::cout << local_tm.tm_mon + 1 << '\n';
		std::cout << local_tm.tm_mday << '\n';

		curTime = (float)local_tm.tm_hour + local_tm.tm_min / 60.0f;

		Engine::TimeOfDayManager& tod = game.GetRenderingPath()->GetTOD();
		tod.SetCurrentDay(1);
		tod.SetCurrentMonth(7);
		tod.SetCurrentTime(curTime);

		Engine::VolumetricClouds& vol = game.GetRenderingPath()->GetVolumetricClouds();
		Engine::VolumetricCloudsData& volData = vol.GetVolumetricCloudsData();

		//volData.cloudCoverage = 0.3f;
		//volData.highCloudsCoverage = 0.4f;


		/*float vertices[] = {
			-0.5f, -0.5f, 0.0f,
			0.5f, -0.5f, 0.0f,
			0.0f, 0.5f, 0.0f
		};

		Engine::VertexAttribute attribs[2] = {};
		// Position
		attribs[0].count = 3;
		attribs[0].offset = 0;
		// Uv
		attribs[1].count = 2;
		attribs[1].offset = 3 * sizeof(float);

		Engine::VertexInputDesc desc = {};
		desc.attribs = { attribs[0], attribs[1] };
		desc.stride = 5 * sizeof(float);

		//Engine::Mesh hexMesh = Engine::MeshDefaults::CreateFromVertices(renderer, desc, 3, vertices, sizeof(vertices));
		//Engine::MaterialInstance *mat = renderer->CreateMaterialInstanceFromBaseMat(game.GetScriptManager(), "Data/", { desc });
		//game.GetModelManager().AddModel(e, hexMesh, mat);*/

		//Engine::Mesh plane = Engine::MeshDefaults::CreateCube(renderer, 16.0f);	
		//Engine::MaterialInstance *mat = renderer->CreateMaterialInstance(game.GetScriptManager(), "Data/Resources/Materials/modelDefault_gi.mat", plane.vao->GetVertexInputDescs());

		//Engine::Entity planeEntity = game.AddEntity();
		//game.GetModelManager().AddModelFromMesh(planeEntity, plane, mat, { glm::vec3(-16.0f, -0.05f, -16.0f), glm::vec3(16.0f, 0.05f, 16.0f) });

		//Engine::Entity lightEntity = game.AddEntity();
		//Engine::PointLight *light = game.GetLightManager().AddPointLight(lightEntity);
		//light->color = glm::vec3(1.0f, 0.0f, 0.0f);

		//Engine::TransformManager &tm = game.GetTransformManager();

		//tm.SetLocalScale(planeEntity, glm::vec3(1.0f, 0.1f, 1.0f));
		//tm.SetLocalPosition(planeEntity, glm::vec3(0.0f, -3.0f, 0.0f));

		for (int i = 0; i < 1; i++)
		{
			//entities[i] = game.AddEntity();
			//Engine::Model *m = game.GetModelManager().AddModel(entities[i], "Data/Levels/eee/crytek-sponza/sponza1.obj");
			/*Engine::MaterialInstance *mat = renderer->CreateMaterialInstance(game.GetScriptManager(), "Data/Resources/Materials/modelDefault_gi.mat", m->GetMeshesAndMaterials()[0].mesh.vao->GetVertexInputDescs());
			for (size_t j = 0; j < m->GetMeshesAndMaterials().size(); j++)
			{
				m->SetMeshMaterial((unsigned short)j, mat);
			}*/


			//float scale = Engine::Random::Float(0.05f, 0.6f);
			//tm.SetLocalPosition(entities[i], glm::vec3(0.0f, 0.0f, -4.0f));
			//tm.SetLocalPosition(entities[i], glm::vec3(float(i) * Engine::Random::Float(1.2f, 1.9f), 0.0f, 0.0f));
			//tm.SetLocalScale(entities[i], glm::vec3(scale));
			//tm.SetLocalRotationEuler(entities[i], glm::vec3(Engine::Random::Float(-70.0f, 70.0f), Engine::Random::Float(-70.0f, 70.0f), 0.0f));
		}

		//tm.SetLocalPosition(lightEntity, glm::vec3(0.0f, 1.0f, -4.0f));

		game.GetMainCamera()->SetMoveSpeed(8.0f);

		return true;
	}
	void Update()
	{
		Application::Update();

		Engine::TransformManager& tm = game.GetTransformManager();

		for (int i = 0; i < 1; i++)
		{
			//glm::vec3 scale = tm.GetLocalScale(entities[i]);
			//scale *= game.GetTimeElapsed();
			//tm.SetLocalScale(entities[i], scale + glm::vec3(game.GetDeltaTime()*0.05f));
		}
	}
	void Render()
	{
		//game.GetRenderingPath()->GetFont().AddText(std::to_string(curTime), glm::vec2(30.0f, 20.0f), glm::vec2(0.2f));

		Application::Render();
	}

private:
	Engine::Entity entities[10];
	float curTime;
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