#pragma once

#include "Graphics\RendererStructs.h"
#include "Graphics\Mesh.h"
#include "Game\ComponentManagers\ScriptManager.h"

#include "include\glm\glm.hpp"

namespace Engine
{
	struct MaterialInstance;
	class Renderer;
	class Buffer;

	struct DebugInstanceData
	{
		glm::mat4 transform;
		glm::vec3 color;
	};

	struct Gizmo
	{
		glm::mat4 xAxis;
		glm::mat4 yAxis;
		glm::mat4 zAxis;
	};

	class DebugDrawManager : public RenderQueueGenerator
	{
	public:
		DebugDrawManager(Renderer *renderer, ScriptManager &scriptManager);
		~DebugDrawManager();

		void Cull(unsigned int passAndFrustumCount, unsigned int *passIds, const Frustum *frustums, std::vector<VisibilityIndices*> &out) override;
		void GetRenderItems(unsigned int passCount, unsigned int *passIds, const VisibilityIndices &visibility, RenderQueue &outQueues) override;

		void Update();
		void Clear();

		void AddCube(const glm::mat4 &t, const glm::vec3 &color = glm::vec3(1.0f));
		void AddSphere(const glm::mat4 &t, const glm::vec3 &color = glm::vec3(1.0f));
		void AddCapsule(const glm::mat4 &t, const glm::vec3 &color = glm::vec3(1.0f));
		void AddTranslationGizmo(const glm::mat4 &xAxis, const glm::mat4 &yAxis, const glm::mat4 &zAxis);

	private:
		unsigned int opaqueQueueID;
		std::vector<DebugInstanceData> cubeData;
		std::vector<DebugInstanceData> sphereData;
		std::vector<Gizmo> gizmos;
		Mesh cubeMesh;
		Mesh sphereMesh;
		Mesh translationGizmoMesh;
		MaterialInstance *matInstance;
		MaterialInstance *gizmoMatInstance;
		Buffer *cubeInstanceBuffer;
		Buffer *sphereInstanceBuffer;
		glm::vec3 gizmoRed;
		glm::vec3 gizmoGreen;
		glm::vec3 gizmoBlue;
	};
}
