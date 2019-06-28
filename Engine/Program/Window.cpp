#include "Window.h"

#include "Log.h"
#include "Graphics\ResourcesLoader.h"
#include "Graphics\VK\VKRenderer.h"
#include "Graphics\Shader.h"
#include "Graphics\GL\GLUtils.h"
#include "Game\UI\UIManager.h"
#include "Utils.h"
#include "Input.h"

namespace Engine
{
	Window::Window()
	{
		editorManager = nullptr;
		inputManager = nullptr;
	}

	bool Window::Init(InputManager *inputManager, GraphicsAPI api, unsigned int width, unsigned int height)
	{
		this->inputManager = inputManager;
		this->width = width;
		this->height = height;

		if (glfwInit() != GLFW_TRUE)
		{
			Log::Print(LogLevel::LEVEL_ERROR, "Failed to initialize GLFW!\n");
			return false;
		}
		else
		{
			Log::Print(LogLevel::LEVEL_INFO, "GLFW successfuly initialized\n");
		}

		//glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);

		if (api == GraphicsAPI::OpenGL)
		{
			glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
			glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
			glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
			//glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE);
			window = glfwCreateWindow(width, height, "Game", nullptr, nullptr);
			glfwMakeContextCurrent(window);
		}
		else if (api == GraphicsAPI::Vulkan)
		{
			glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
			window = glfwCreateWindow(width, height, "Game", nullptr, nullptr);
		}
		else if (api == GraphicsAPI::D3D11)
		{
			glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
			window = glfwCreateWindow(width, height, "Game", nullptr, nullptr);
		}

		glfwSetWindowPos(window, 550, 50);

		glfwSetWindowUserPointer(window, this);

		glfwSetKeyCallback(window, WindowKeyboardCallback);
		glfwSetCursorPosCallback(window, WindowMouseCallback);
		glfwSetMouseButtonCallback(window, WindowMouseButtonCallback);
		glfwSetWindowFocusCallback(window, WindowFocusCallback);
		glfwSetCharCallback(window, WindowCharCallback);
		glfwSetFramebufferSizeCallback(window, WindowFramebufferSizeCallback);

		glfwSetCursorPos(window, width / 2.0f, height / 2.0f);

		const GLFWvidmode *mode = glfwGetVideoMode(glfwGetPrimaryMonitor());
		monitorWidth = mode->width;
		monitorHeight = mode->height;

		// For they're only needed for the editor
#ifdef EDITOR
		glfwSetScrollCallback(window, WindowScrollCallback);
#else
		glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
#endif

		return true;
	}

	void Window::Dispose()
	{
		glfwTerminate();
	}

	void Window::UpdateInput()
	{
		inputManager->Reset();
		inputManager->Update();

		glfwPollEvents();
	}

	bool Window::WasResized()
	{
		if (wasResized)
		{
			wasResized = false;
			return true;
		}
		return false;
	}

	void Window::UpdateKeys(int key, int scancode, int action, int mods)
	{
		/*if (key == GLFW_KEY_ESCAPE)
			glfwSetWindowShouldClose(window, GLFW_TRUE);
		*/
		inputManager->UpdateKeys(key, scancode, action, mods);

#ifdef EDITOR
		editorManager->UpdateKeys(key, action, mods);
#endif

		if (key == GLFW_KEY_F11 && action == GLFW_RELEASE)
		{
			int count = 0;
			const GLFWvidmode *vidModes = glfwGetVideoModes(glfwGetPrimaryMonitor(), &count);

			int maxRefreshRate = 0;

			width = 0;
			height = 0;

			if (!isFullscreen)
			{
				for (int i = 0; i < count; i++)
				{
					if ((unsigned int)vidModes[i].width > width && (unsigned int)vidModes[i].height > height)
					{
						width = vidModes[i].width;
						height = vidModes[i].height;

						if (vidModes[i].refreshRate > maxRefreshRate)
							maxRefreshRate = vidModes[i].refreshRate;
					}
					//std::cout << vidModes[i].width << " " << vidModes[i].height << " " << vidModes[i].refreshRate << " " << vidModes[i].redBits << " " << vidModes[i].greenBits << " " << vidModes[i].blueBits << std::endl;
				}
				std::cout << width << " " << height << " " << maxRefreshRate << std::endl;
				glfwSetWindowMonitor(window, glfwGetPrimaryMonitor(), 0, 0, width, height, maxRefreshRate);

				isFullscreen = true;
			}
			else
			{
				width = 1280;
				height = 720;

				// Find max refresh rate for 1280x720
				for (int i = 0; i < count; i++)
				{
					if (vidModes[i].width == width && vidModes[i].height == height && vidModes[i].refreshRate > maxRefreshRate)
					{
						maxRefreshRate = vidModes[i].refreshRate;

						if (maxRefreshRate == 60)			// We dont want more than 60hz
							break;
					}
				}
				//std::cout << "Windowed refresh rate:" << maxRefreshRate << std::endl;
				glfwSetWindowMonitor(window, nullptr, 550, 50, width, height, maxRefreshRate);

				isFullscreen = false;
			}

			wasResized = true;
		}
	}

	void Window::UpdateMousePosition(float xpos, float ypos)
	{
#ifdef EDITOR
		glm::vec2 viewportPos = editorManager->GetGameViewportPos();
		glm::vec2 mousePosInGameView = glm::vec2(xpos - viewportPos.x, ypos - viewportPos.y);		// Subtract the viewport so we get the correct mouse position in the game view dock
		inputManager->SetMousePosition(mousePosInGameView);
#else
		// Better solution for this?  Maybe move input to UIManager?
		/*bool visible = game.GetUIManager()->IsCursorVisible();
		if (visible)										// Constrain mouse movement if the mouse is visible
		{
			glm::vec2 cursorPos = game.GetUIManager()->GetCursorPos();
			cursorPos.x += xpos - lastXpos;
			cursorPos.y += ypos - lastYpos;
			lastXpos = xpos;
			lastYpos = ypos;

			unsigned int width = renderer->GetWidth();
			unsigned int height = renderer->GetHeight();

			if (cursorPos.x < 0.0f)
				cursorPos.x = 0.0f;
			else if (cursorPos.x > width)
				cursorPos.x = width;

			if (cursorPos.y < 0.0f)
				cursorPos.y = 0.0f;
			else if (cursorPos.y > height)
				cursorPos.y = height;

			game.GetUIManager()->SetCursorPos(cursorPos);
			inputManager.SetMousePosition(cursorPos);
		}
		else
		{*/
		inputManager->SetMousePosition(glm::vec2(xpos, ypos));
		//}	

#endif
	}

	void Window::UpdateMouseButtonState(int button, int action, int mods)
	{
		inputManager->SetMouseButtonState(button, action);

#ifdef EDITOR
		editorManager->UpdateMouse(button, action);
#endif
	}

	void Window::UpdateScroll(double xoffset, double yoffset)
	{
		inputManager->SetScrollWheelYOffset((float)yoffset);

#ifdef EDITOR
		editorManager->UpdateScroll(yoffset);
#endif
	}

	void Window::UpdateChar(unsigned int c)
	{
		inputManager->UpdateChar(c);

#ifdef EDITOR
		editorManager->UpdateChar(c);
#endif
	}

	void Window::UpdateFocus(int focused)
	{
		if (focused == GLFW_TRUE)
			isFocused = true;
		else if (focused == GLFW_FALSE)
			isFocused = false;

#ifdef EDITOR
		if (isFocused && editorManager)		// Check if the editorManager is not null because this function might be called right at the start before the editor manager is set
			editorManager->OnFocus();
#endif	
	}

	void Window::UpdateFramebufferSize(int width, int height)
	{
		//std::cout << "width : " << width << "    "  << height << '\n';
		this->width = static_cast<unsigned int>(width);
		this->height = static_cast<unsigned int>(height);

		if (width == 0 && height == 0)
			isMinimized = true;
		else
		{
			isMinimized = false;
			wasResized = true;
		}
	}
}