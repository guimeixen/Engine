#pragma once

#include "Graphics\Camera\Camera.h"
#include "Graphics\Mesh.h"
#include "Game\ComponentManagers\ScriptManager.h"

#include <vector>

namespace Engine
{
	class Renderer;
	struct MaterialInstance;
	class Texture;

	class ProjectedGridWater
	{
	public:
		ProjectedGridWater();

		void Init(Renderer *renderer, ScriptManager &scriptManager, float waterHeight);
		void Update(Camera *camera, float deltaTime);
		void Render(Renderer *renderer);
		void Dispose();

		void SetWaterHeight(float height);

		MaterialInstance *GetMaterialInstance() const { return mat; }

		const glm::mat4 &GetViewFrame() const { return viewFrame; }
		const glm::vec4 &GetViewCorner0() const { return viewCorners[0]; }
		const glm::vec4 &GetViewCorner1() const { return viewCorners[1]; }
		const glm::vec4 &GetViewCorner2() const { return viewCorners[2]; }
		const glm::vec4 &GetViewCorner3() const { return viewCorners[3]; }
		glm::vec4 GetNormalMapOffset() const { return glm::vec4(normalMapOffset0.x, normalMapOffset0.y, normalMapOffset1.x, normalMapOffset1.y); }
		float GetWaterHeight() const { return waterHeight; }

	private:
		void IntersectFrustumWithWaterPlane(Camera *camera);
		void IntersectFrustumEdgeWaterPlane(const glm::vec3 &start, const glm::vec3 &end);
		bool SegmentPlaneIntersection(const glm::vec3 &a, const glm::vec3 &b, const glm::vec3 &n, float d, glm::vec3 &q);

	private:
		std::vector<glm::vec3> intersectionPoints;
		float waterHeight;
		Plane waterTopPlane;
		Plane waterBottomPlane;
		Mesh projectedGridMesh;
		MaterialInstance *mat;
		glm::vec4 viewCorners[4];
		glm::mat4 viewFrame;

		glm::vec3 frustumCornersWorld[8];

		glm::vec2 normalMapOffset0;
		glm::vec2 normalMapOffset1;

		Texture *normalMap;
		Texture *foamTexture;
	};
}
