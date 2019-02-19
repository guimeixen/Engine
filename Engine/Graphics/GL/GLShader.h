#pragma once

#include "Graphics\Shader.h"

#include "include\glew\glew.h"

#include <map>
#include <vector>

namespace Engine
{
	class GLShader : public Shader
	{
	public:
		GLShader(const std::string &defines, const std::string &vertexName, const std::string &fragmentName);
		GLShader(const std::string &defines, const std::string &vertexName, const std::string &geometryName, const std::string &fragmentName);
		GLShader(const std::string &defines, const std::string &computePath);
		~GLShader();

		void Use() const;
		void Unuse() const;
		unsigned int GetProgram() const { return program; }

		void SetModelMatrix(const glm::mat4 &matrix);
		void SetInstanceDataOffset(int offset);

		void SetMat4(const std::string &name, const glm::mat4& matrix);
		void SetMat3(const std::string &name, const glm::mat3& matrix);
		void SetVec4(const std::string &name, const glm::vec4& vec);
		void SetVec3(const std::string &name, const glm::vec3& vec);
		void SetVec2(const std::string &name, const glm::vec2 &v);
		void SetFloat(const std::string &name, float x);
		void SetBool(const std::string &name, bool value);
		void SetInt(const std::string &name, int value);

		void SetMat4Array(const std::string &name, const float *data, unsigned int count);

	private:
		std::string ReadFile(std::ifstream &file, const std::string &dir);
		void SetUniformLocations();

	private:
		GLuint program;
		GLint modelMatrixLoc;
		GLint instanceDataOffsetLoc;
		std::map<std::string, int> uniforms;
	};
}
