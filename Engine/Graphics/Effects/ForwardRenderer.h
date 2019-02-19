#pragma once

#include "Graphics/UniformBufferTypes.h"
#include "RenderingPath.h"

namespace Engine
{
	class Game;
	class Renderer;
	class Camera;
	class Framebuffer;
	class Buffer;

	class ForwardRenderer : public RenderingPath
	{
	public:
		ForwardRenderer();

		void Init(Game *game);
		void Dispose();
		void Resize(unsigned int width, unsigned int height);
		void Render();

	private:
		void SetupHDRPass();		
		void SetupPostProcessPass();

		void PerformHDRPass();
		void PerformPostProcessPass();

	private:
		Buffer *pointLightsUBO;
	};
}
