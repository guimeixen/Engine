#pragma once

#include "RendererStructs.h"

#include <string>
#include <filesystem>

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
		std::filesystem::file_time_type lastVertexWriteTime;
		std::filesystem::file_time_type lastFragmentWriteTime;
		std::filesystem::file_time_type lastGeometryWriteTime;
		std::filesystem::file_time_type lastComputeWriteTime;
	};
}
