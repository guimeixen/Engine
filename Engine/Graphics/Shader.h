#pragma once

#include "RendererStructs.h"

#include <string>
#include <filesystem>

namespace Engine
{
	enum class ShaderType
	{
		VERTEX,
		FRAGMENT,
		GEOMETRY,
		COMPUTE
	};

	class ShaderProgram
	{
	public:
		virtual ~ShaderProgram() {}

		virtual void CheckIfModifiedAndReload() = 0;

		bool IsCompiled() const { return isCompiled; }

	protected:
		std::string defines;
		std::string vertexName;
		std::string fragmentName;
		std::string geometryName;
		std::string computeName;

		std::filesystem::file_time_type lastVertexWriteTime;
		std::filesystem::file_time_type lastFragmentWriteTime;
		std::filesystem::file_time_type lastGeometryWriteTime;
		std::filesystem::file_time_type lastComputeWriteTime;

		bool isCompiled;
	};
}
