#pragma once

#include "RendererStructs.h"

#include <string>

namespace Engine
{
	class ShaderProgram
	{
	public:
		virtual ~ShaderProgram() {}

		virtual void Reload() = 0;
		virtual bool CheckIfModified() = 0;

	protected:
		std::string defines;
	};
}
