#include "Input.h"

namespace Engine
{
	InputManager *Input::inputManager = nullptr;

	InputManager::InputManager()
	{
		// Clear all the keys
		for (size_t i = 0; i < 512; i++)
		{
			keys[i].state = false;
			keys[i].justReleased = false;
			keys[i].justPressed = false;
		}

		Input::inputManager = this;

		mouseButtonsState[0] = { false, false, false };
		mouseButtonsState[1] = { false, false, false };

		mouseMoved = false;

		mousePosition = glm::vec2();
		charUpdated = false;

		scrollWheelY = 0.0f;
	}

	void InputManager::Update()
	{
		//charUpdated = false;

		mouseMoved = false;

		for (int i = 0; i < 512; i++)
		{
			keys[i].justReleased = false;
			keys[i].justPressed = false;
		}

		mouseButtonsState[0].justPressed = false;
		mouseButtonsState[1].justPressed = false;
	}

	void InputManager::Reset()
	{
		mouseButtonsState[0].justReleased = false;
		mouseButtonsState[1].justReleased = false;
	}

	int InputManager::AnyKeyPressed() const
	{
		for (int i = 0; i < 512; i++)
		{
			if (keys[i].state)
				return i;
		}
		return -1;
	}

	bool InputManager::GetLastChar(unsigned char &c)
	{
		c = lastChar;

		if (charUpdated)
		{
			charUpdated = false;
			return true;
		}

		return false;
	}

	bool InputManager::IsKeyPressed(int keycode) const
	{
		return keys[keycode].state;
	}

	bool InputManager::WasKeyPressed(int keycode) const
	{
		return keys[keycode].justPressed;
	}

	bool InputManager::WasKeyReleased(int keycode) const
	{
		return keys[keycode].justReleased;
	}

	bool InputManager::WasMouseButtonReleased(int button)
	{
		return mouseButtonsState[button].justReleased;
	}

	bool InputManager::MouseMoved() const
	{
		return mouseMoved;
	}

	bool InputManager::IsMousePressed(int button) const
	{
		return mouseButtonsState[button].justPressed;
	}

	bool InputManager::IsMouseButtonDown(int button) const
	{
		return mouseButtonsState[button].state;
	}

	void InputManager::SetMousePosition(const glm::vec2 &pos)
	{
		mousePosition = pos;
		mouseMoved = true;
	}

	void InputManager::UpdateKeys(int key, int scancode, int action, int mods)
	{
		if (key >= 0 && key < 512)
		{
			if (action == KEY_PRESSED)
			{
				keys[key].state = true;
				keys[key].justReleased = false;
				keys[key].justPressed = true;
			}
			else if (action == KEY_RELEASED)
			{
				keys[key].state = false;
				keys[key].justReleased = true;
				keys[key].justPressed = false;
			}
		}
	}

	void InputManager::UpdateChar(unsigned char c)
	{
		lastChar = c;
		charUpdated = true;
	}

	void InputManager::SetMouseButtonState(int button, int action)
	{
		if (button < 0 || button > 1)
			return;

		if (button == MOUSE_BUTTON_LEFT)
		{
			if (action == KEY_PRESSED)
			{
				mouseButtonsState[0].state = true;
				mouseButtonsState[0].justReleased = false;
				mouseButtonsState[0].justPressed = true;
			}
			else if (action == KEY_RELEASED)
			{
				mouseButtonsState[0].state = false;
				mouseButtonsState[0].justReleased = true;
				mouseButtonsState[0].justPressed = false;
			}
		}
		else if (button == MOUSE_BUTTON_RIGHT)
		{
			if (action == KEY_PRESSED)
			{
				mouseButtonsState[1].state = true;
				mouseButtonsState[1].justReleased = false;
				mouseButtonsState[1].justPressed = true;
			}
			else if (action == KEY_RELEASED)
			{
				mouseButtonsState[1].state = false;
				mouseButtonsState[1].justReleased = true;
				mouseButtonsState[1].justPressed = false;
			}
		}
	}
	void InputManager::SetScrollWheelYOffset(float yoffset)
	{
		scrollWheelY += yoffset;
	}

	void InputManager::UpdateVitaButtons(int buttons)
	{
		this->buttons = buttons;
	}

	void InputManager::UpdateVitaSticks(unsigned char leftStickX, unsigned char leftStickY, unsigned char rightStickX, unsigned char rightStickY)
	{
		this->leftStickX = (float)leftStickX;
		this->leftStickX -= (this->leftStickX - 128.0f) / 128.0f;
		
		this->leftStickY = (float)leftStickY;
		this->leftStickY -= (this->leftStickY - 128.0f) / 128.0f;

		this->rightStickX = (float)rightStickX;
		this->rightStickX -= (this->rightStickX - 128.0f) / 128.0f;

		this->rightStickY = (float)rightStickY;
		this->rightStickY -= (this->rightStickY - 128.0f) / 128.0f;
	}
}
