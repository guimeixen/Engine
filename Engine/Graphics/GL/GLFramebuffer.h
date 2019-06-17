#pragma once

#include "include\glew\glew.h"

#include "Graphics\Framebuffer.h"

namespace Engine
{
	class GLFramebuffer : public Framebuffer
	{
	public:
		GLFramebuffer(const FramebufferDesc &desc);
		~GLFramebuffer();

		void Resize(const FramebufferDesc &desc) override;
		void Clear() const override;
		void Bind() const override;

		static void SetDefault(int width, int height, int x = 0, int y = 0);

		GLuint GetHandle() const { return id; }

	private:
		void Create(const FramebufferDesc &desc);
		void Dispose();

	private:
		GLuint id;
		GLuint rboID;
		GLbitfield clearMask;
	};
}
