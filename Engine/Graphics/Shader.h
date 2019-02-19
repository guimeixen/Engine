#pragma once

#include "RendererStructs.h"

#include "include\glm\glm.hpp"

#include <string>

namespace Engine
{
	class Shader
	{
	public:
		virtual ~Shader() {}

		virtual void Use() const;
		virtual void Unuse() const;
	};
}
