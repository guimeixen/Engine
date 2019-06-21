#pragma once

#include "RendererStructs.h"

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
