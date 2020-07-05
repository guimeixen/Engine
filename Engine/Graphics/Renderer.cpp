#include "Renderer.h"

#ifndef VITA
#include "GL/GLUtils.h"
#include "VK/VKUtils.h"
#include "GL/GLRenderer.h"
#include "VK/VKRenderer.h"
#include "D3D11/D3D11Renderer.h"
#include "D3D11/D3D11Utils.h"

#include "include/GLFW/glfw3.h"

#else
#include "GXM/GXMRenderer.h"
#include "GXM/GXMUtils.h"
#endif

#include "Material.h"

#ifdef _WIN32
#define GLFW_EXPOSE_NATIVE_WIN32
#include "GLFW\glfw3native.h"
#endif

namespace Engine
{
	GraphicsAPI Renderer::currentAPI;

	Renderer *Renderer::Create(GLFWwindow *window, GraphicsAPI api, FileManager *fileManager, unsigned int width, unsigned int height, unsigned int monitorWidth, unsigned int monitorHeight)
	{
		Renderer *renderer = nullptr;

#ifndef VITA
		if (api == GraphicsAPI::OpenGL)
		{
			renderer = new GLRenderer(fileManager, static_cast<GLuint>(width), static_cast<GLuint>(height));
			if (!renderer->Init())
				return nullptr;
			/*GLint flags;
			glGetIntegerv(GL_CONTEXT_FLAGS, &flags);
			if (flags & GL_CONTEXT_FLAG_DEBUG_BIT)
			{
			glEnable(GL_DEBUG_OUTPUT);
			glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
			glDebugMessageCallback((GLDEBUGPROC)glutils::DebugOutput, nullptr);
			glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, GL_TRUE);
			}*/

			renderer->AddGlobalDefine("OPENGL_API");

			return renderer;
		}
		else if (api == GraphicsAPI::Vulkan)
		{
			renderer = new VKRenderer(fileManager, window, width, height, monitorWidth, monitorHeight);
			if (!renderer->Init())
				return nullptr;

			renderer->AddGlobalDefine("VULKAN_API");

			return renderer;
		}
#else
		if (api == GraphicsAPI::GXM)
		{
			renderer = new GXMRenderer(fileManager);
			if (!renderer->Init())
				return nullptr;

			return renderer;
		}
#endif

#ifdef _WIN32
		else if (api == GraphicsAPI::D3D11)
		{
			HWND hwnd = glfwGetWin32Window(window);
			currentAPI = api;
			renderer = new D3D11Renderer(hwnd, width, height);
			if (!renderer->Init())
				return nullptr;

			return renderer;
		}
#endif

		return nullptr;
	}

	unsigned int Renderer::GetBlendFactorValue(BlendFactor blendFactor)
	{
#ifndef VITA
		switch (currentAPI)
		{
		case GraphicsAPI::OpenGL:
			return glutils::GetGLBlendFactor(blendFactor);
			break;
		case GraphicsAPI::Vulkan:
			return vkutils::GetVKBlendFactor(blendFactor);
			break;
		case GraphicsAPI::D3D11:
			return d3d11utils::GetBlendFactor(blendFactor);
			break;
		case GraphicsAPI::GXM:
			break;
		}

		return 0;
#else
		return gxmutils::GetBlendFactorValue(blendFactor);
#endif
	}

	unsigned int Renderer::GetTopologyValue(Topology topology)
	{
#ifndef VITA
		switch (currentAPI)
		{
		case GraphicsAPI::OpenGL:
			return glutils::GetGLTopology(topology);
			break;
		case GraphicsAPI::Vulkan:
			return vkutils::GetVKTopology(topology);
			break;
		case GraphicsAPI::D3D11:
			return d3d11utils::GetTopology(topology);
			break;
		case GraphicsAPI::GXM:
			break;
		}

		return 0;
#else
		return gxmutils::GetTopology(topology);
#endif
	}

	unsigned int Renderer::GetDepthFunc(const std::string &func)
	{
#ifndef VITA
		switch (currentAPI)
		{
		case GraphicsAPI::OpenGL:
			return glutils::DepthFuncFromString(func);
			break;
		case GraphicsAPI::Vulkan:
			return vkutils::GetDepthFunc(func);
			break;
		case GraphicsAPI::D3D11:
			return d3d11utils::GetDepthFunc(func);
			break;
		case GraphicsAPI::GXM:
			break;
		}

		return 0;
#else
		return gxmutils::GetDepthFunc(func);
#endif
	}

	unsigned int Renderer::GetCullMode(const std::string &mode)
	{
#ifndef VITA
		switch (currentAPI)
		{
		case GraphicsAPI::OpenGL:
			return glutils::CullFaceFromString(mode);
			break;
		case GraphicsAPI::Vulkan:
			return vkutils::GetCullMode(mode);
			break;
		case GraphicsAPI::D3D11:
			return d3d11utils::GetCullMode(mode);
			break;
		case GraphicsAPI::GXM:
			break;
		}

		return 0;
#else
		return gxmutils::GetCullMode(mode);
#endif
	}

	unsigned int Renderer::GetFrontFace(const std::string &face)
	{
#ifndef VITA
		switch (currentAPI)
		{
		case GraphicsAPI::OpenGL:
			return glutils::FrontFaceFromString(face);
			break;
		case GraphicsAPI::Vulkan:
			return vkutils::GetFrontFace(face);
			break;
		case GraphicsAPI::D3D11:
			return d3d11utils::GetFrontFace(face);
			break;
		case GraphicsAPI::GXM:
			break;
		}

		return 0;
#else
		return gxmutils::GetFrontFace(face);
#endif
	}

	std::vector<VisibilityIndices> Renderer::Cull(unsigned int queueAndFrustumCount, unsigned int *queueIDs, const Frustum *frustums)
	{
		std::vector<VisibilityIndices> visibility(renderQueueGenerators.size() * queueAndFrustumCount);
		std::vector<VisibilityIndices*> visibilityTemp(queueAndFrustumCount);

		for (size_t i = 0; i < renderQueueGenerators.size(); i++)
		{
			for (unsigned int j = 0; j < queueAndFrustumCount; j++)
				visibilityTemp[j] = &visibility[j * renderQueueGenerators.size() + i];

			renderQueueGenerators[i]->Cull(queueAndFrustumCount, queueIDs, frustums, visibilityTemp);
		}

		return visibility;
	}

	void Renderer::CopyVisibilityToQueue(std::vector<VisibilityIndices> &visibility, unsigned int srcQueueIndex, unsigned int dstQueueIndex)
	{
		if (dstQueueIndex * renderQueueGenerators.size() >= (unsigned int)visibility.size())
		{
			//size_t queueCount = visibility.size() / renderQueueGenerators.size();

			for (size_t i = 0; i < renderQueueGenerators.size(); i++)
			{
				visibility.push_back(visibility[srcQueueIndex * renderQueueGenerators.size() + i]);
			}	
		}
		else
		{
			for (size_t i = 0; i < renderQueueGenerators.size(); i++)
			{
				visibility[dstQueueIndex * renderQueueGenerators.size() + i] = visibility[srcQueueIndex * renderQueueGenerators.size() + i];
			}
		}
	}

	void Renderer::CreateRenderQueues(unsigned int passAndFrustumCount, unsigned int *passIds, const std::vector<VisibilityIndices> &visibility, RenderQueue *outQueues)
	{
		/*std::vector<VisibilityIndices> visibility(renderQueueGenerators.size() * passAndFrustumCount);
		std::vector<VisibilityIndices*> visibilityTemp(passAndFrustumCount);

		for (size_t i = 0; i < renderQueueGenerators.size(); i++)
		{
			for (unsigned int j = 0; j < passAndFrustumCount; j++)
				visibilityTemp[j] = &visibility[j * renderQueueGenerators.size() + i];

			renderQueueGenerators[i]->Cull(passAndFrustumCount, passIds, frustums, visibilityTemp);
		}*/

		std::vector<std::vector<RenderQueue>> queues(renderQueueGenerators.size());
		static const unsigned short MAX_PASSES = 8;
		unsigned int totalRenderItems[MAX_PASSES] = { 0 };


		for (size_t i = 0; i < renderQueueGenerators.size(); i++)
		{
			queues[i].resize(passAndFrustumCount);

			for (unsigned int j = 0; j < passAndFrustumCount; j++)
			{
				if (visibility[j * renderQueueGenerators.size() + i].size() > 0)
					renderQueueGenerators[i]->GetRenderItems(1, &passIds[j], visibility[j * renderQueueGenerators.size() + i], queues[i][j]);
			}
		}

		for (size_t j = 0; j < renderQueueGenerators.size(); j++)
		{
			for (unsigned int k = 0; k < passAndFrustumCount; k++)
			{
				totalRenderItems[k] += queues[j][k].size();
			}
		}

		for (unsigned int j = 0; j < passAndFrustumCount; j++)
		{
			outQueues[j].clear();
			if (totalRenderItems[j] > 0 && outQueues[j].size() == 0)
				outQueues[j].resize(totalRenderItems[j]);
		}

		for (unsigned int j = 0; j < passAndFrustumCount; j++)
		{
			size_t offset = 0;

			for (size_t k = 0; k < renderQueueGenerators.size(); k++)
			{
				size_t queueSize = queues[k][j].size();
				memcpy(outQueues[j].data() + offset, queues[k][j].data(), sizeof(RenderItem) * queueSize);
				offset += queueSize;
			}
		}
	}

	void Renderer::RemoveRenderQueueGenerator(RenderQueueGenerator *generator)
	{
		for (auto it = renderQueueGenerators.begin(); it != renderQueueGenerators.end(); it++)
		{
			if (*it == generator)
			{
				renderQueueGenerators.erase(it);
				break;
			}
		}
	}

	bool Renderer::RemoveTexture(Texture *t)
	{
		for (auto it = textures.begin(); it != textures.end(); it++)
		{
			Texture *tex = it->second;

			if (tex == t && tex->GetRefCount() == 1)
			{
				if (tex->GetRefCount() == 1)
				{
					textures.erase(it);
					tex->RemoveReference();
					return true;
				}
				else
				{
					tex->RemoveReference();
					return false;
				}
			}
		}

		return false;
	}

	bool Renderer::RemoveMaterialInstance(MaterialInstance *m)
	{
		for (auto it = materialInstances.begin(); it != materialInstances.end(); it++)
		{
			if ((*it) == m)
			{
				materialInstances.erase(it);
				delete m;
				m = nullptr;
				return true;
			}
		}

		return false;
	}
}
