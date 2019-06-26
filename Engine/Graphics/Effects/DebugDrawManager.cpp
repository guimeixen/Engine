#include "DebugDrawManager.h"

#include "Graphics/MeshDefaults.h"
#include "Graphics/ResourcesLoader.h"
#include "Graphics/Material.h"
#include "Graphics/Buffers.h"
#include "Graphics/VertexArray.h"
#include "Graphics/Renderer.h"

#include "Program/Utils.h"
#include "Program/StringID.h"
#include "Program/Log.h"

namespace Engine
{
	DebugDrawManager::DebugDrawManager(Renderer *renderer, ScriptManager &scriptManager)
	{
		opaqueQueueID = SID("opaque");

		cubeMesh = {};
		sphereMesh = {};
		translationGizmoMesh = {};

		gizmoRed = glm::vec3(1.0f, 0.0f, 0.0f);
		gizmoGreen = glm::vec3(0.0f, 1.0f,0.0f);
		gizmoBlue = glm::vec3(0.0f, 0.0f, 1.0f);

		cubeMesh = MeshDefaults::CreateCube(renderer, 0.5f, true, true);
		sphereMesh = MeshDefaults::CreateSphere(renderer, 0.5f, true);
		translationGizmoMesh = MeshDefaults::CreateCube(renderer);

		cubeInstanceBuffer = renderer->CreateVertexBuffer(nullptr, 1000 * sizeof(DebugInstanceData), BufferUsage::DYNAMIC);
		sphereInstanceBuffer = renderer->CreateVertexBuffer(nullptr, 1000 * sizeof(DebugInstanceData), BufferUsage::DYNAMIC);

		if (cubeMesh.vao)
			cubeMesh.vao->AddVertexBuffer(cubeInstanceBuffer);
		if(sphereMesh.vao)
			sphereMesh.vao->AddVertexBuffer(sphereInstanceBuffer);

		if (cubeMesh.vao)
		{
			const std::vector<VertexInputDesc> &cubeDesc = cubeMesh.vao->GetVertexInputDescs();
			matInstance = renderer->CreateMaterialInstanceFromBaseMat(scriptManager, "Data/Resources/Materials/debugDraw_mat.lua", cubeDesc);
		}
		if (translationGizmoMesh.vao)
			gizmoMatInstance = renderer->CreateMaterialInstanceFromBaseMat(scriptManager, "Data/Resources/Materials/gizmo_mat.lua", translationGizmoMesh.vao->GetVertexInputDescs());

		Log::Print(LogLevel::LEVEL_INFO, "Init Debug draw manager\n");
	}

	DebugDrawManager::~DebugDrawManager()
	{
		if (cubeMesh.vao)
			delete cubeMesh.vao;

		if (sphereMesh.vao)
			delete sphereMesh.vao;

		if (translationGizmoMesh.vao)
			delete translationGizmoMesh.vao;

		Log::Print(LogLevel::LEVEL_INFO, "Disposing Debug draw manager\n");
	}

	void DebugDrawManager::Cull(unsigned int passAndFrustumCount, unsigned int *passIds, const Frustum *frustums, std::vector<VisibilityIndices*> &out)
	{
		for (unsigned int i = 0; i < passAndFrustumCount; i++)
		{
			// No frustum culling performed for now
			if (cubeMesh.instanceCount > 0 || sphereMesh.instanceCount > 0 || gizmos.size() > 0)
				out[i]->push_back(0);
		}
	}

	void DebugDrawManager::GetRenderItems(unsigned int passCount, unsigned int *passIds, const VisibilityIndices &visibility, RenderQueue &outQueues)
	{
		for (unsigned int i = 0; i < passCount; i++)
		{
			if (passIds[i] == opaqueQueueID)
			{
				RenderItem ri = {};

				if (cubeMesh.instanceCount > 0)
				{
					ri.mesh = &cubeMesh;
					ri.matInstance = matInstance;
					ri.shaderPass = 0;
					outQueues.push_back(ri);
				}
				if (sphereMesh.instanceCount > 0)
				{
					ri.mesh = &sphereMesh;
					ri.matInstance = matInstance;
					ri.shaderPass = 0;
					outQueues.push_back(ri);
				}

				if (gizmos.size() > 0)
				{
					for (size_t i = 0; i < gizmos.size(); i++)
					{
						const Gizmo &g = gizmos[i];

						RenderItem ri = {};
						ri.mesh = &translationGizmoMesh;
						ri.matInstance = gizmoMatInstance;
						ri.shaderPass = 0;

						ri.transform = &g.xAxis;
						ri.materialData = &gizmoRed;
						ri.materialDataSize = sizeof(gizmoRed);
						outQueues.push_back(ri);

						ri.transform = &g.yAxis;
						ri.materialData = &gizmoGreen;
						ri.materialDataSize = sizeof(gizmoGreen);
						outQueues.push_back(ri);

						ri.transform = &g.zAxis;
						ri.materialData = &gizmoBlue;
						ri.materialDataSize = sizeof(gizmoBlue);
						outQueues.push_back(ri);
					}
				}

				break;
			}
		}
	}

	void DebugDrawManager::Update()
	{
		cubeMesh.instanceCount = 0;

		if (cubeData.size() > 0)
		{
			cubeMesh.instanceCount = cubeData.size();
			cubeInstanceBuffer->Update(cubeData.data(), cubeData.size() * sizeof(DebugInstanceData), 0);
		}

		cubeData.clear();

		sphereMesh.instanceCount = 0;

		if (sphereData.size() > 0)
		{
			sphereMesh.instanceCount = sphereData.size();
			sphereInstanceBuffer->Update(sphereData.data(), sphereData.size() * sizeof(DebugInstanceData), 0);
		}

		sphereData.clear();
	}

	void DebugDrawManager::Clear()
	{
		gizmos.clear();
	}

	void DebugDrawManager::AddCube(const glm::mat4 &t, const glm::vec3 &color)
	{
		cubeData.push_back({ t, color });
	}

	void DebugDrawManager::AddSphere(const glm::mat4 &t, const glm::vec3 &color)
	{
		sphereData.push_back({ t,color });
	}

	void DebugDrawManager::AddCapsule(const glm::mat4 &t, const glm::vec3 &color)
	{
	}

	void DebugDrawManager::AddTranslationGizmo(const glm::mat4 &xAxis, const glm::mat4 &yAxis, const glm::mat4 &zAxis)
	{
		Gizmo g = {};
		g.xAxis = xAxis;
		g.yAxis = yAxis;
		g.zAxis = zAxis;
		gizmos.push_back(g);
	}
}
