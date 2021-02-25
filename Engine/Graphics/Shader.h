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

		virtual bool CheckIfModified() = 0;
		virtual void Reload() = 0;

		bool IsCompiled() const { return isCompiled; }

		const std::string& GetVertexName() const { return vertexName; }
		const std::string& GetFragmentName() const { return fragmentName; }
		const std::string& GetGeometryName() const { return geometryName; }
		const std::string& GetComputeName() const { return computeName; }

		bool HasGeometry() const { return geometryName.length() != 0; }

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
