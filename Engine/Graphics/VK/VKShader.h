#pragma once

#include "Graphics/Shader.h"

#include <vulkan/vulkan.h>

namespace Engine
{
	class VKShader : public ShaderProgram
	{
	public:
		VKShader(unsigned int id, const std::string &vertexName, const std::string &fragmentName, const std::string &defines);
		VKShader(unsigned int id, const std::string &vertexName, const std::string &geometryName, const std::string &fragmentName, const std::string &defines);
		VKShader(unsigned int id, const std::string &computeName, const std::string &defines);

		void Compile(VkDevice device);
		bool CreateShaderModule(VkDevice device);
		bool CheckIfModified() override;
		void Reload() override;
		void Dispose(VkDevice device);

		VkPipelineShaderStageCreateInfo GetVertexStageInfo();
		VkPipelineShaderStageCreateInfo GetGeometryStageInfo();
		VkPipelineShaderStageCreateInfo GetFragmentStageInfo();
		VkPipelineShaderStageCreateInfo GetComputeStageInfo();

	private:
		void ReadShaderFile(const std::string &path, ShaderType type);
		void WriteShaderFileWithDefines(ShaderType type);
		void CompileShader(const std::string &path, ShaderType type);

	private:
		unsigned int id;
		std::string idStr;

		bool vertexExists;
		bool geometryExists;
		bool fragmentExists;
		bool computeExists;

		bool vertexNeedsCompile;
		bool geometryNeedsCompile;
		bool fragmentNeedsCompile;
		bool computeNeedsCompile;

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
