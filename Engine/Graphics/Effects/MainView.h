#pragma once

#include "Graphics\RendererStructs.h"
#include "Graphics\Mesh.h"
#include "Graphics\Effects\CascadedShadowMap.h"
#include "Graphics\Material.h"
#include "Graphics\Font.h"
#include "ProjectedGridWater.h"
#include "TimeOfDayManager.h"
#include "VCTGI.h"
#include "Graphics\UniformBufferTypes.h"
#include "Graphics\FrameGraph.h"
#include "VolumetricClouds.h"

#include "Game\ComponentManagers\ScriptManager.h"

namespace Engine
{
	class Framebuffer;
	class Camera;
	class Buffer;

	class MainView
	{
	public:
		MainView();
		~MainView();

		void Render();

	private:
		void PerformHDRPass();
		void PerformPostProcessPass();

		void GenerateLightning();
		void AddBranch(const glm::vec3 &startPoint, float lengthMultiplier, float startingBranchIntensity);

	private:
		Game *game;
		Renderer *renderer;
		Camera *mainCamera;
		Camera uiCamera;
		Framebuffer *hdrFB;
		Framebuffer *csmFB;
		Framebuffer *editorFB;
		Framebuffer *brightPassFB;
		Framebuffer *downsampleFB[3];
		Framebuffer *upsampleFB[2];
		unsigned int width;
		unsigned int height;

		FrameGraph frameGraph;

		RenderQueue renderQueues[7];

		bool enableUI;
		bool wireframe;

		// Lighting

		MaterialInstance *debugMatInstance;
		struct DebugMaterialData
		{
			//glm::vec2 translation;
			//float scale;
			int isShadowMap;
		};
		DebugMaterialData debugMatData;

		// Skybox
		/*Mesh skyboxMesh;
		MaterialInstance *skyboxMatInstance;
		unsigned int skyboxShaderPass;*/


		// Water
		Framebuffer *reflectionFB;
		Framebuffer *refractionFB;
		Mesh waterMesh;
		MaterialInstance *waterMatInstance;
		unsigned int waterShaderPass;

		// Lightning
		Mesh lightningMesh;
		MaterialInstance *lightningMatInstance;
		unsigned int divisions = 32;
		struct LightningVertex
		{
			glm::vec3 pos;
			float intensity;
		};
		std::vector<LightningVertex> lightningVertices;
		int branchLevel = 0;
		bool performStrike = false;
		float lightningTimer = 0.0f;
		float lightningAlpha = 0.0f;

		// Low poly water;
		/*Mesh lowPolyWaterMesh;
		MaterialInstance *lowPolyWaterMatInstance;
		unsigned int lowPolyWaterShaderPass;*/
	};
}
