#pragma once

#include "include\glew\glew.h"

#include "Graphics\Texture.h"
#include "Graphics\MaterialInfo.h"
#include "Graphics\UniformBufferTypes.h"

namespace Engine
{
	namespace glutils
	{
		GLenum UsageToGL(BufferUsage usage);
		GLint WrapToGL(TextureWrap wrap);
		GLenum FormatToGL(TextureFormat format);
		GLint InternalFormatToGL(TextureInternalFormat internalFormat);
		GLenum TypeToGL(TextureDataType type);
		unsigned int GetGLBlendFactor(BlendFactor factor);
		unsigned int GetGLTopology(Topology topology);
		void __stdcall DebugOutput(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar *message, void *userParam);

		unsigned int CullFaceFromString(const std::string &str);
		unsigned int FrontFaceFromString(const std::string &str);
		unsigned int DepthFuncFromString(const std::string &str);
	}
}
