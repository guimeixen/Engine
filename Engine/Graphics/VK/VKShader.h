#pragma once

#include "Graphics\Shader.h"

#include <vulkan\vulkan.h>

#include <string>

namespace Engine
{
	enum class ShaderType
	{
		VERTEX,
		FRAGMENT,
		GEOMETRY,
		COMPUTE
	};

	class VKShader : public Shader
	{
	public:
		VKShader(unsigned int id, const std::string &vertexName, const std::string &fragmentName, const std::string &defines);
		VKShader(unsigned int id, const std::string &vertexName, const std::string &geometryName, const std::string &fragmentName, const std::string &defines);
		VKShader(unsigned int id, const std::string &computeName, const std::string &defines);
		~VKShader();

		void Compile(VkDevice device);
		void CreateShaderModule(VkDevice device);

		void Dispose(VkDevice device);

		VkPipelineShaderStageCreateInfo GetVertexStageInfo();
		VkPipelineShaderStageCreateInfo GetGeometryStageInfo();
		VkPipelineShaderStageCreateInfo GetFragmentStageInfo();
		VkPipelineShaderStageCreateInfo GetComputeStageInfo();

		const std::string &GetVertexName() const { return vertexName; }
		const std::string &GetFragmentName() const { return fragmentName; }
		const std::string &GetGeometryName() const { return geometryName; }
		const std::string &GetComputeName() const { return computeName; }

		bool HasGeometry() const { return geometryName.length() != 0; }

	private:
		void CompileShader(const std::string &path, ShaderType type);

	private:
		unsigned int id;
		std::string idStr;
		std::string defines;

		bool compiled;

		bool vertexExists;
		bool geometryExists;
		bool fragmentExists;
		bool computeExists;

		bool vertexNeedsCompile;
		bool geometryNeedsCompile;
		bool fragmentNeedsCompile;
		bool computeNeedsCompile;

		std::string vertexName;
		std::string fragmentName;
		std::string geometryName;
		std::string computeName;

		std::string vertexCode;
		std::string geometryCode;
		std::string fragmentCode;
		std::string computeCode;

		VkShaderModule vertexModule;
		VkShaderModule geometryModule;
		VkShaderModule fragmentModule;
		VkShaderModule computeModule;
	};
}
