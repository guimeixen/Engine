#include "Input.h"

#include "Program/StringID.h"
#include "Program/FileManager.h"
#include "Program/Log.h"
#include "Program/Serializer.h"

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
		mouseButtonsState[2] = { false, false, false };

		mouseMoved = false;
		lastButtons = 0;
		lastChar = 0;
		mousePosition = glm::vec2();
		charUpdated = false;

		scrollWheelY = 0.0f;

		lastKeyPressed = Keys::KEY_SPACE;

		buttons = 0;
		leftStickX = 0.0f;
		leftStickY = 0.0f;
		rightStickX = 0.0f;
		rightStickY = 0.0f;

		keysToString[Keys::KEY_SPACE] = "Space";
		keysToString[Keys::KEY_0] = "0";
		keysToString[Keys::KEY_1] = "1";
		keysToString[Keys::KEY_2] = "2";
		keysToString[Keys::KEY_3] = "3";
		keysToString[Keys::KEY_4] = "4";
		keysToString[Keys::KEY_5] = "5";
		keysToString[Keys::KEY_6] = "6";
		keysToString[Keys::KEY_7] = "7";
		keysToString[Keys::KEY_8] = "8";
		keysToString[Keys::KEY_9] = "9";
		keysToString[Keys::KEY_A] = "A";
		keysToString[Keys::KEY_B] = "B";
		keysToString[Keys::KEY_C] = "C";
		keysToString[Keys::KEY_D] = "D";
		keysToString[Keys::KEY_E] = "E";
		keysToString[Keys::KEY_F] = "F";
		keysToString[Keys::KEY_G] = "G";
		keysToString[Keys::KEY_H] = "H";
		keysToString[Keys::KEY_I] = "I";
		keysToString[Keys::KEY_J] = "J";
		keysToString[Keys::KEY_K] = "K";
		keysToString[Keys::KEY_L] = "L";
		keysToString[Keys::KEY_M] = "M";
		keysToString[Keys::KEY_N] = "N";
		keysToString[Keys::KEY_O] = "O";
		keysToString[Keys::KEY_P] = "P";
		keysToString[Keys::KEY_Q] = "Q";
		keysToString[Keys::KEY_R] = "R";
		keysToString[Keys::KEY_S] = "S";
		keysToString[Keys::KEY_T] = "T";
		keysToString[Keys::KEY_U] = "U";
		keysToString[Keys::KEY_V] = "V";
		keysToString[Keys::KEY_W] = "W";
		keysToString[Keys::KEY_X] = "X";
		keysToString[Keys::KEY_Y] = "Y";
		keysToString[Keys::KEY_Z] = "Z";
		keysToString[Keys::KEY_ESCAPE] = "Escape";
		keysToString[Keys::KEY_ENTER] = "Enter";
		keysToString[Keys::KEY_TAB] = "Tab";
		keysToString[Keys::KEY_BACKSPACE] = "Backspace";
		keysToString[Keys::KEY_DEL] = "Delete";
		keysToString[Keys::KEY_F1] = "F1";
		keysToString[Keys::KEY_F2] = "F2";
		keysToString[Keys::KEY_F3] = "F3";
		keysToString[Keys::KEY_F4] = "F4";
		keysToString[Keys::KEY_F5] = "F5";
		keysToString[Keys::KEY_F6] = "F6";
		keysToString[Keys::KEY_F7] = "F7";
		keysToString[Keys::KEY_F8] = "F8";
		keysToString[Keys::KEY_F9] = "F9";
		keysToString[Keys::KEY_F10] = "F10";
		keysToString[Keys::KEY_F11] = "F11";
		keysToString[Keys::KEY_F12] = "F12";
		keysToString[Keys::KEY_LEFT_SHIFT] = "Left shift";
		keysToString[Keys::KEY_LEFT_CONTROL] = "Left control";

		vitaButtonsToString[VitaButtons::VITA_SELECT] = "Select";
		vitaButtonsToString[VitaButtons::VITA_START] = "Start";
		vitaButtonsToString[VitaButtons::VITA_UP] = "Up";
		vitaButtonsToString[VitaButtons::VITA_RIGHT] = "Right";
		vitaButtonsToString[VitaButtons::VITA_DOWN] = "Down";
		vitaButtonsToString[VitaButtons::VITA_LEFT] = "Left";
		vitaButtonsToString[VitaButtons::VITA_LTRIGGER] = "Left trigger";
		vitaButtonsToString[VitaButtons::VITA_RTRIGGER] = "Right trigger";
		vitaButtonsToString[VitaButtons::VITA_TRIANGLE] = "Triangle";
		vitaButtonsToString[VitaButtons::VITA_CIRCLE] = "Circle";
		vitaButtonsToString[VitaButtons::VITA_CROSS] = "Cross";
		vitaButtonsToString[VitaButtons::VITA_SQUARE] = "Square";
		vitaButtonsToString[VitaButtons::VITA_PSBUTTON] = "PS button";
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

	void InputManager::LoadInputMappings(FileManager *fileManager, const std::string &path)
	{
		inputMappings.clear();
		
		Serializer s(fileManager);
		s.OpenForReading(path);

		if (s.IsOpen())
		{
			unsigned int size = 0;
			s.Read(size);

			InputMapping im = {};
			std::string name;
			int temp = 0;
			unsigned short tempShort = 0;

			for (unsigned int i = 0; i < size; i++)
			{
				im = {};

				s.Read(name);
				s.Read(temp);
				im.mouseButton = (MouseButtonType)temp;
				s.Read(tempShort);
				im.negativeKey = (Keys)tempShort;
				s.Read(tempShort);
				im.positiveKey = (Keys)tempShort;
				s.Read(temp);
				im.negativeVitaButton = (VitaButtons)temp;
				s.Read(temp);
				im.positiveVitaButton = (VitaButtons)temp;
				s.Read(im.useLeftAnalogueStickX);
				s.Read(im.useLeftAnalogueStickY);
				s.Read(im.useRightAnalogueStickX);
				s.Read(im.useRightAnalogueStickY);

				inputMappings[name] = im;
			}
		}
		else
		{
			// Create default input mappings
			InputMapping horizontal = {};
			horizontal.mouseButton = MouseButtonType::None;
			horizontal.positiveKey = Keys::KEY_D;
			horizontal.negativeKey = Keys::KEY_A;
			horizontal.useLeftAnalogueStickX = true;

			InputMapping vertical = {};
			vertical.mouseButton = MouseButtonType::None;
			vertical.positiveKey = Keys::KEY_W;
			vertical.negativeKey = Keys::KEY_S;
			vertical.useLeftAnalogueStickY = true;

			InputMapping fire = {};
			fire.mouseButton = MouseButtonType::Left;
			fire.positiveVitaButton = VitaButtons::VITA_RTRIGGER;

			inputMappings["Horizontal"] = horizontal;
			inputMappings["Vertical"] = vertical;
			inputMappings["Fire"] = fire;
		}

		s.Close();
	}

	void InputManager::SaveInputMappings(FileManager* fileManager, const std::string& path)
	{
		Serializer s(fileManager);
		s.OpenForWriting();

		if (s.IsOpen())
		{
			s.Write((unsigned int)inputMappings.size());

			for (auto it = inputMappings.begin(); it != inputMappings.end(); it++)
			{
				const InputMapping& im = it->second;
				
				s.Write(it->first);
				s.Write(im.mouseButton);
				s.Write(im.negativeKey);
				s.Write(im.positiveKey);
				s.Write(im.negativeVitaButton);
				s.Write(im.positiveVitaButton);
				s.Write(im.useLeftAnalogueStickX);
				s.Write(im.useLeftAnalogueStickY);
				s.Write(im.useRightAnalogueStickX);
				s.Write(im.useRightAnalogueStickY);
			}
		}

		s.Save(path);
		s.Close();
	}

	void InputManager::AddInputMapping(const std::string& name, const InputMapping& newMapping)
	{
		// Don't do anything if we already one with the same name
		if (inputMappings.find(name) != inputMappings.end())
			return;

		inputMappings[name] = newMapping;
	}

	int InputManager::AnyKeyPressed() const
	{
		for (int i = 0; i < 512; i++)
		{
			if (keys[i].justPressed)
				return i;
		}
		return 0;
	}

	bool InputManager::AnyMouseButtonPressed() const
	{
		for (int i = 0; i < NUM_MOUSE_BUTTON_TYPES; i++)
		{
			if (mouseButtonsState[i].state)
				return true;
		}

		return false;
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

	Keys InputManager::GetLastKeyPressed() const
	{
		return lastKeyPressed;
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

	MouseButtonType InputManager::GetLastMouseButtonPressed() const
	{
		return lastMouseButtonPressed;
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
				lastKeyPressed = (Keys)key;
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

	void InputManager::SetMouseButtonState(MouseButtonType button, int action)
	{
		if (button < MouseButtonType::Left || button > MouseButtonType::Middle)
			return;

		if (action == KEY_PRESSED)
		{
			lastMouseButtonPressed = button;
			mouseButtonsState[button].state = true;
			mouseButtonsState[button].justReleased = false;
			mouseButtonsState[button].justPressed = true;
		}
		else if (action == KEY_RELEASED)
		{
			mouseButtonsState[button].state = false;
			mouseButtonsState[button].justReleased = true;
			mouseButtonsState[button].justPressed = false;
		}
	}
	void InputManager::SetScrollWheelYOffset(float yoffset)
	{
		scrollWheelY += yoffset;
	}

	void InputManager::UpdateVitaButtons(int buttons)
	{
		lastButtons = this->buttons;
		this->buttons = buttons;
	}

	void InputManager::UpdateVitaSticks(unsigned char leftStickX, unsigned char leftStickY, unsigned char rightStickX, unsigned char rightStickY)
	{
		this->leftStickX = (float)leftStickX;
		this->leftStickX = (this->leftStickX - 128.0f) / 128.0f;
		
		this->leftStickY = (float)leftStickY;
		this->leftStickY = (this->leftStickY - 128.0f) / 128.0f;
		this->leftStickY *= -1.0f;			// Flip the Y to -1 on the bottom and 1 on the top because the analogue stick is -1 on the top and 1 on the bottom

		this->rightStickX = (float)rightStickX;
		this->rightStickX = (this->rightStickX - 128.0f) / 128.0f;

		this->rightStickY = (float)rightStickY;
		this->rightStickY = (this->rightStickY - 128.0f) / 128.0f;
	}

	bool InputManager::IsVitaButtonDown(int button)
	{
		return buttons & button;
	}

	bool InputManager::WasVitaButtonReleased(int button)
	{
		if ((lastButtons & button) == true && (buttons & button) == false)
			return true;

		return false;
	}

	float InputManager::GetAxis(const std::string &name)
	{
		// If using strings as the key becomes slows, then we could try ints
		//unsigned int id = SID(name);
		
		std::map<std::string, InputMapping>::iterator it = inputMappings.find(name);			// We use find instead of [] because otherwise we would insert InputMappings during gameplay if they didn't exist

		if (it != inputMappings.end())
		{
			const InputMapping& im = (*it).second;

#ifdef VITA
		if (im.useLeftAnalogueStickX && (leftStickX > 0.1f || leftStickX < -0.1f))
			return leftStickX;

		if (im.useLeftAnalogueStickY && (leftStickY > 0.1f || leftStickY < -0.1f))
			return leftStickY;
#else
		if (IsKeyPressed(im.positiveKey))
			return 1.0f;

		if (IsKeyPressed(im.negativeKey))
			return -1.0f;
#endif	
		}

		return 0.0f;
	}

	bool InputManager::GetAction(const std::string &name)
	{
		std::map<std::string, InputMapping>::iterator it = inputMappings.find(name);			// We use find instead of [] because we don't want to insert an InputMapping if it doesnt's exist

		if (it != inputMappings.end())
		{
			const InputMapping& im = (*it).second;

#ifdef VITA
			if (buttons & im.positiveVitaButton)
				return true;
#else
			if (IsKeyPressed(im.positiveKey))
				return true;
#endif			
		}

		return false;
	}
}
