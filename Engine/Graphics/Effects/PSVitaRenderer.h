#pragma once

#include "RenderingPath.h"

namespace Engine
{
	class PSVitaRenderer : public RenderingPath
	{
	public:
		PSVitaRenderer();

		void Init(Game *game);
		void Dispose();
		void Render();

	private:
		Buffer *mainLightUBO;
		DirLight mainDirectionalLight;
	};
}
