#pragma once

#include "RenderingPath.h"

namespace Engine
{
	class ForwardPlusRenderer : public RenderingPath
	{
	public:
		ForwardPlusRenderer();

		void Init(Game *game);
		void Dispose();
		void Resize(unsigned int width, unsigned int height);
		void Render();

	private:
		void SetupComputeFrustumsPass();
		void SetupDepthPrepass();
		void SetupLightCullingPass();
		void SetupHDRPass();
		void SetupPostProcessPass();

		void PerformHDRPass();
		void PerformPostProcessPass();

	private:
		struct ShaderPlane
		{
			glm::vec3 normal;
			float d;
		};
		struct ShaderFrustum
		{
			ShaderPlane planes[4];
		};

		const unsigned int LIGHT_TILE_SIZE = 16;
		const unsigned int MAX_LIGHTS = 64;
		unsigned int curLightCount = 0;
		glm::uvec2 numFrustums;

		MaterialInstance *frustumsMat;
		MaterialInstance *lightCullingMat;

		Buffer *frustumsSSBO;
		Buffer *lightListSSBO;
		Buffer *opaqueLightIndexListSSBO;
		Buffer *opaqueLightIndexCounterSSBO;
		Texture *lightGrid;

		Framebuffer *depthPrepassFB;

		unsigned int frustumsPassNumWorkGroupsX;
		unsigned int frustumsPassNumWorkGroupsY;

		unsigned int depthPrepassQueueID;
	};
}
