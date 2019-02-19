#include "GLUtils.h"

#include <iostream>

namespace Engine
{
	namespace glutils
	{
		GLenum UsageToGL(BufferUsage usage)
		{
			switch (usage)
			{
			case BufferUsage::STATIC:
				return GL_STATIC_DRAW;
				break;
			case BufferUsage::STREAM:
				return GL_STREAM_DRAW;
				break;
			case BufferUsage::DYNAMIC:
				return GL_DYNAMIC_DRAW;
				break;
			}

			return GL_STATIC_DRAW;
		}

		GLint WrapToGL(TextureWrap wrap)
		{
			switch (wrap)
			{
			case TextureWrap::REPEAT:
				return GL_REPEAT;
				break;
			case TextureWrap::CLAMP:
				return GL_CLAMP;
				break;
			case TextureWrap::MIRRORED_REPEAT:
				return GL_MIRRORED_REPEAT;
				break;
			case TextureWrap::CLAMP_TO_EDGE:
				return GL_CLAMP_TO_EDGE;
				break;
			case TextureWrap::CLAMP_TO_BORDER:
				return GL_CLAMP_TO_BORDER;
				break;
			}

			return 0;
		}

		GLenum FormatToGL(TextureFormat format)
		{
			switch (format)
			{
			case TextureFormat::RGB:
				return GL_RGB;
				break;
			case TextureFormat::RGBA:
				return GL_RGBA;
				break;
			case TextureFormat::DEPTH_COMPONENT:
				return GL_DEPTH_COMPONENT;
				break;
			case TextureFormat::RED:
				return GL_RED;
				break;
			}

			return 0;
		}

		GLint InternalFormatToGL(TextureInternalFormat internalFormat)
		{
			switch (internalFormat)
			{
			case TextureInternalFormat::RGB8:
				return GL_RGB8;
				break;
			case TextureInternalFormat::RGBA8:
				return GL_RGBA8;
				break;
			case TextureInternalFormat::RGB16F:
				return GL_RGB16F;
				break;
			case TextureInternalFormat::RGBA16F:
				return GL_RGBA16F;
				break;
			case TextureInternalFormat::SRGB8:
				return GL_SRGB8;
				break;
			case TextureInternalFormat::SRGB8_ALPHA8:
				return GL_SRGB8_ALPHA8;
				break;
			case TextureInternalFormat::DEPTH_COMPONENT16:
				return GL_DEPTH_COMPONENT16;
				break;
			case TextureInternalFormat::DEPTH_COMPONENT24:
				return GL_DEPTH_COMPONENT24;
				break;
			case TextureInternalFormat::DEPTH_COMPONENT32:
				return GL_DEPTH_COMPONENT32;
				break;
			case TextureInternalFormat::RED8:
				return GL_R8;
				break;
			case TextureInternalFormat::R16F:
				return GL_R16F;
				break;
			case TextureInternalFormat::R32UI:
				return GL_R32UI;
				break;
			case TextureInternalFormat::RG32UI:
				return GL_RG32UI;
				break;
			}

			return 0;
		}

		GLenum TypeToGL(TextureDataType type)
		{
			switch (type)
			{
			case TextureDataType::FLOAT:
				return GL_FLOAT;
				break;
			case TextureDataType::UNSIGNED_BYTE:
				return GL_UNSIGNED_BYTE;
				break;
			case TextureDataType::UNSIGNED_INT:
				return GL_UNSIGNED_INT;
				break;
			}

			return 0;
		}

		unsigned int GetGLBlendFactor(BlendFactor factor)
		{
			switch (factor)
			{
			case ZERO:
				return GL_ZERO;
				break;
			case ONE:
				return GL_ONE;
				break;
			case SRC_ALPHA:
				return GL_SRC_ALPHA;
				break;
			case DST_ALPHA:
				return GL_DST_ALPHA;
				break;
			case SRC_COLOR:
				return GL_SRC_COLOR;
				break;
			case DST_COLOR:
				return GL_DST_COLOR;
				break;
			case ONE_MINUS_SRC_ALPHA:
				return GL_ONE_MINUS_SRC_ALPHA;
				break;
			case ONE_MINUS_SRC_COLOR:
				return GL_ONE_MINUS_SRC_COLOR;
				break;
			}

			return 0;
		}

		unsigned int GetGLTopology(Topology topology)
		{
			switch (topology)
			{
			case TRIANGLES:
				return GL_TRIANGLES;
				break;
			case TRIANGLE_STRIP:
				return GL_TRIANGLE_STRIP;
				break;
			case LINES:
				return GL_LINES;
				break;
			case LINE_TRIP:
				return GL_LINE_STRIP;
				break;
			default:
				break;
			}

			return 0;
		}

		void __stdcall DebugOutput(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar *message, void *userParam)
		{
			// ignore non-significant error/warning codes
			if (id == 131169 || id == 131185 || id == 131218 || id == 131204)
				return;

			std::cout << "---------------" << '\n';
			std::cout << "Debug message (" << id << "): " << message << '\n';

			switch (source)
			{
			case GL_DEBUG_SOURCE_API:             std::cout << "Source: API"; break;
			case GL_DEBUG_SOURCE_WINDOW_SYSTEM:   std::cout << "Source: Window System"; break;
			case GL_DEBUG_SOURCE_SHADER_COMPILER: std::cout << "Source: Shader Compiler"; break;
			case GL_DEBUG_SOURCE_THIRD_PARTY:     std::cout << "Source: Third Party"; break;
			case GL_DEBUG_SOURCE_APPLICATION:     std::cout << "Source: Application"; break;
			case GL_DEBUG_SOURCE_OTHER:           std::cout << "Source: Other"; break;
			}
			std::cout << '\n';

			switch (type)
			{
			case GL_DEBUG_TYPE_ERROR:               std::cout << "Type: Error"; break;
			case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR: std::cout << "Type: Deprecated Behaviour"; break;
			case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:  std::cout << "Type: Undefined Behaviour"; break;
			case GL_DEBUG_TYPE_PORTABILITY:         std::cout << "Type: Portability"; break;
			case GL_DEBUG_TYPE_PERFORMANCE:         std::cout << "Type: Performance"; break;
			case GL_DEBUG_TYPE_MARKER:              std::cout << "Type: Marker"; break;
			case GL_DEBUG_TYPE_PUSH_GROUP:          std::cout << "Type: Push Group"; break;
			case GL_DEBUG_TYPE_POP_GROUP:           std::cout << "Type: Pop Group"; break;
			case GL_DEBUG_TYPE_OTHER:               std::cout << "Type: Other"; break;
			}
			std::cout << '\n';

			switch (severity)
			{
			case GL_DEBUG_SEVERITY_HIGH:         std::cout << "Severity: high"; break;
			case GL_DEBUG_SEVERITY_MEDIUM:       std::cout << "Severity: medium"; break;
			case GL_DEBUG_SEVERITY_LOW:          std::cout << "Severity: low"; break;
			case GL_DEBUG_SEVERITY_NOTIFICATION: std::cout << "Severity: notification"; break;
			}
			std::cout << '\n';
			std::cout << '\n';
		}

		unsigned int CullFaceFromString(const std::string &str)
		{
			if (str == "front")
				return GL_FRONT;
			else if (str == "back")
				return GL_BACK;

			return GL_BACK;
		}

		unsigned int FrontFaceFromString(const std::string &str)
		{
			if (str == "ccw")
				return GL_CCW;
			else if (str == "cw")
				return GL_CW;

			return GL_CCW;
		}

		unsigned int DepthFuncFromString(const std::string &str)
		{
			if (str == "less")
				return GL_LESS;
			else if (str == "lequal")
				return GL_LEQUAL;

			return GL_LESS;
		}

	}
}
