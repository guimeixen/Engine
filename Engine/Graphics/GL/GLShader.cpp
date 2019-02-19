#include "GLShader.h"

//#include "VariableTypes.h"

#include "include\glm\gtc\type_ptr.hpp"

#include <string>
#include <fstream>
#include <sstream>
#include <iostream>

namespace Engine
{
	GLShader::GLShader(const std::string &defines, const std::string &vertexName, const std::string &fragmentName)
	{
		std::ifstream vertexFile, fragmentFile;

		std::string vertexPath = "Data/Shaders/GL/" + vertexName + ".vert";
		std::string fragmentPath = "Data/Shaders/GL/" + fragmentName + ".frag";

		vertexFile.open(vertexPath);
		fragmentFile.open(fragmentPath);

		if (!vertexFile.is_open() || !fragmentFile.is_open())
		{
			std::cout << "Error! Failed to load shader: \n\t" << vertexPath << "\n\t" << fragmentPath << "\n";
			return;
		}

		std::string dir = vertexPath.substr(0, vertexPath.find_last_of("/\\"));

		std::string vertexCode = ReadFile(vertexFile, dir);
		std::string fragmentCode = ReadFile(fragmentFile, dir);

		vertexCode.insert(13, defines);
		fragmentCode.insert(13, defines);

		GLuint vs = glCreateShader(GL_VERTEX_SHADER);
		GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);

		int status;
		char log[1024];

		const char *vsCode = vertexCode.c_str();
		const char *fsCode = fragmentCode.c_str();

		glShaderSource(vs, 1, &vsCode, NULL);
		glShaderSource(fs, 1, &fsCode, NULL);

		glCompileShader(vs);
		glCompileShader(fs);

		glGetShaderiv(vs, GL_COMPILE_STATUS, &status);
		if (!status)
		{
			glGetShaderInfoLog(vs, 1024, NULL, log);
			std::cout << "Vertex shader compilation failed:" << vertexPath << "\n" << std::string(log) << "\n";
		}

		glGetShaderiv(fs, GL_COMPILE_STATUS, &status);
		if (!status)
		{
			glGetShaderInfoLog(fs, 1024, NULL, log);
			std::cout << "Fragment shader compilation failed:" << fragmentPath << "\n" << std::string(log) << "\n";
		}

		program = glCreateProgram();
		glAttachShader(program, vs);
		glAttachShader(program, fs);

		glLinkProgram(program);
		glGetProgramiv(program, GL_LINK_STATUS, &status);
		if (!status)
		{
			glGetProgramInfoLog(program, 1024, NULL, log);
			std::cout << vertexName << " " << fragmentName << " Shader program linking failed:\n" << std::string(log) << "\n";
		}

		glDeleteShader(vs);
		glDeleteShader(fs);

		SetUniformLocations();
	}

	GLShader::GLShader(const std::string &defines, const std::string &vertexName, const std::string &geometryName, const std::string &fragmentName)
	{
		std::ifstream vertexFile, geometryFile, fragmentFile;

		std::string vertexPath = "Data/Shaders/GL/" + vertexName + ".vert";
		std::string geometryPath = "Data/Shaders/GL/" + geometryName + ".geom";
		std::string fragmentPath = "Data/Shaders/GL/" + fragmentName + ".frag";

		vertexFile.open(vertexPath);
		geometryFile.open(geometryPath);
		fragmentFile.open(fragmentPath);

		if (!vertexFile.is_open() || !geometryFile.is_open() || !fragmentFile.is_open())
		{
			std::cout << "Error! Failed to load shader: \n\t" << vertexPath << "\n\t" << geometryPath << "\n\t" << fragmentPath << "\n";
			return;
		}

		std::string dir = vertexPath.substr(0, vertexPath.find_last_of("/\\"));

		std::string vertexCode = ReadFile(vertexFile, dir);
		std::string geometryCode = ReadFile(geometryFile, dir);
		std::string fragmentCode = ReadFile(fragmentFile, dir);

		// Insert the defines after the  #version ###
		vertexCode.insert(13, defines);
		geometryCode.insert(13, defines);
		fragmentCode.insert(13, defines);

		GLuint vs = glCreateShader(GL_VERTEX_SHADER);
		GLuint gs = glCreateShader(GL_GEOMETRY_SHADER);
		GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);

		int status;
		char log[1024];

		const char *vsCode = vertexCode.c_str();
		const char *gsCode = geometryCode.c_str();
		const char *fsCode = fragmentCode.c_str();

		glShaderSource(vs, 1, &vsCode, NULL);
		glShaderSource(gs, 1, &gsCode, NULL);
		glShaderSource(fs, 1, &fsCode, NULL);

		glCompileShader(vs);
		glCompileShader(gs);
		glCompileShader(fs);

		glGetShaderiv(vs, GL_COMPILE_STATUS, &status);
		if (!status)
		{
			glGetShaderInfoLog(vs, 1024, NULL, log);
			std::cout << "Vertex shader compilation failed:\n\t" << vertexPath << "\n\n" << log << "\n";
		}

		glGetShaderiv(gs, GL_COMPILE_STATUS, &status);
		if (!status)
		{
			glGetShaderInfoLog(gs, 1024, NULL, log);
			std::cout << "Geometry shader compilation failed:\n\t" << geometryPath << "\n\n" << log << "\n";
		}

		glGetShaderiv(fs, GL_COMPILE_STATUS, &status);
		if (!status)
		{
			glGetShaderInfoLog(fs, 1024, NULL, log);
			std::cout << "Fragment shader compilation failed:\n\t" << fragmentPath << "\n\n" << log << "\n";
		}

		program = glCreateProgram();
		glAttachShader(program, vs);
		glAttachShader(program, gs);
		glAttachShader(program, fs);

		glLinkProgram(program);
		glGetProgramiv(program, GL_LINK_STATUS, &status);
		if (!status)
		{
			glGetProgramInfoLog(program, 1024, NULL, log);
			std::cout << "Shader program linking failed:\n\t" << log << "\n";
		}

		glDeleteShader(vs);
		glDeleteShader(gs);
		glDeleteShader(fs);

		SetUniformLocations();
	}

	GLShader::GLShader(const std::string &defines, const std::string &computePath)
	{
		std::ifstream computeFile;

		std::string path = "Data/Shaders/GL/" + computePath + ".comp";

		computeFile.open(path);

		if (!computeFile.is_open())
		{
			std::cout << "Failed load compute shader: \n\t" << path << '\n';
			return;
		}

		std::string dir = path.substr(0, path.find_last_of("/\\"));
		std::string computeCode = ReadFile(computeFile, dir);

		// Insert the defines after the  #version ### core
		computeCode.insert(13, defines);

		unsigned int cs = glCreateShader(GL_COMPUTE_SHADER);

		int status;
		char log[1024];

		const char *csCode = computeCode.c_str();

		glShaderSource(cs, 1, &csCode, NULL);
		glCompileShader(cs);


		glGetShaderiv(cs, GL_COMPILE_STATUS, &status);
		if (!status)
		{
			glGetShaderInfoLog(cs, 1024, NULL, log);
			std::cout << "Compue shader compilation failed:\n\t" << computePath << "\n\t" << log << '\n';
		}

		program = glCreateProgram();
		glAttachShader(program, cs);
		glLinkProgram(program);

		glGetProgramiv(program, GL_LINK_STATUS, &status);
		if (!status)
		{
			glGetProgramInfoLog(program, 1024, NULL, log);
			std::cout << "Compute shader linking failed:\n\t" << computePath << "\n\t" << log << '\n';
		}

		glDeleteShader(cs);

		SetUniformLocations();
	}

	GLShader::~GLShader()
	{
		glDeleteProgram(program);
	}

	std::string GLShader::ReadFile(std::ifstream &file, const std::string &dir)
	{
		std::string src, line;

		while (std::getline(file, line))
		{
			if (line.substr(0, 8) == "#include")
			{
				std::string includePath = dir + "/" + line.substr(9);
				std::ifstream includeFile(includePath);

				if (includeFile.is_open())
				{
					src += ReadFile(includeFile, dir);
					includeFile.close();
				}
				else
				{
					std::cout << "Failed to include shader:\n\t" + includePath << "\n";
				}
			}
			else
				src += line + "\n";
		}

		return src;
	}

	void GLShader::SetUniformLocations()
	{
		int nrUniforms;
		glGetProgramiv(program, GL_ACTIVE_UNIFORMS, &nrUniforms);

		char buffer[128];
		int size;
		GLenum glType;

		// Used to set the sampler values
		//Use();
		int samplerIndex = 0;

		for (int i = 0; i < nrUniforms; i++)
		{
			glGetActiveUniform(program, i, sizeof(buffer), 0, &size, &glType, buffer);

			std::string uniformName(buffer);

			//	size_t pos = uniformName.find_last_of("[");

			/*if (pos != std::string::npos)
			{
			std::string startingName = uniformName.substr(0, pos);
			uniformName = startingName;

			for (int i = 0; i < size; i++)
			{
			uniformName += "[" + std::to_string(i) + "]";
			uniforms.insert({ uniformName.c_str(), glGetUniformLocation(program, uniformName.c_str()) });
			uniformName = startingName;
			}
			}
			else
			{*/
			uniforms.insert({ buffer, glGetUniformLocation(program, buffer) });
			//	}

			/*if (defines == "#define ANIMATED\n")
			{
			GLint i = glGetUniformLocation(program, "boneTransforms");
			std::cout << "boneTransforms location: " << i << '\n';
			}*/

			// Needs to be below the uniforms.insert above otherwise if we set the sampler binding it won't be in the map so it will pass -1
			// If using this call the Use function commented above
			/*if (glType == GL_SAMPLER_2D)
			{
			SetInt(buffer, samplerIndex);
			samplerIndex++;
			}*/

			//std::cout << uniformName << " ->  size: " << size << " type:" << glType << "\n";
		}

		//Unuse();

		modelMatrixLoc = glGetUniformLocation(program, "toWorldSpace");
		instanceDataOffsetLoc = glGetUniformLocation(program, "instanceDataOffset");
	}

	void GLShader::Use() const
	{
		glUseProgram(program);
	}

	void GLShader::Unuse() const
	{
		glUseProgram(0);
	}

	void GLShader::SetModelMatrix(const glm::mat4 &matrix)
	{
		if (modelMatrixLoc != -1)
			glUniformMatrix4fv(modelMatrixLoc, 1, GL_FALSE, glm::value_ptr(matrix));
	}

	void GLShader::SetInstanceDataOffset(int offset)
	{
		glUniform1i(instanceDataOffsetLoc, offset);
	}

	void GLShader::SetMat4(const std::string &name, const glm::mat4 &matrix)
	{
		/*if (modelMatrixLoc != -1)
			glUniformMatrix4fv(modelMatrixLoc, 1, GL_FALSE, glm::value_ptr(matrix));*/
	}

	void GLShader::SetMat3(const std::string &name, const glm::mat3 &matrix)
	{
		glUniformMatrix3fv(uniforms[name], 1, GL_FALSE, glm::value_ptr(matrix));
	}

	void GLShader::SetVec4(const std::string &name, const glm::vec4 &vec)
	{
		glUniform4f(uniforms[name], vec.x, vec.y, vec.z, vec.w);
	}

	void GLShader::SetVec3(const std::string &name, const glm::vec3 &vec)
	{
		glUniform3f(uniforms[name], vec.x, vec.y, vec.z);
	}

	void GLShader::SetVec2(const std::string &name, const glm::vec2 &v)
	{
		glUniform2f(uniforms[name], v.x, v.y);
	}

	void GLShader::SetFloat(const std::string &name, const float x)
	{
		glUniform1f(uniforms[name], x);
	}

	void GLShader::SetBool(const std::string &name, bool value)
	{
		glUniform1i(uniforms[name], value);
	}

	void GLShader::SetInt(const std::string &name, int value)
	{
		std::map<std::string, int>::const_iterator pos = uniforms.find(name);			// Check if we have the uniform in the map otherwise it would add a new one with the default value of 0, which could
		if (pos == uniforms.end())														// overwrite an already set uniform and for it to not happen we use -1
		{
			glUniform1i(-1, value);
		}
		else
		{
			glUniform1i(uniforms[name], value);
		}
	}

	void GLShader::SetMat4Array(const std::string &name, const float *data, unsigned int count)
	{
		glUniformMatrix4fv(uniforms[name], (GLsizei)count, GL_FALSE, (GLfloat*)data);
	}
}
