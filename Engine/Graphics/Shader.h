#pragma once

#include "RendererStructs.h"

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
