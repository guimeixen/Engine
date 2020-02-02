#pragma once

#include "include/glm/glm.hpp"

#include <vector>
#include <unordered_map>

#define KEY_PRESSED			1
#define KEY_RELEASED		0
#define MOUSE_BUTTON_LEFT	0
#define MOUSE_BUTTON_RIGHT	1

namespace Engine
{
	enum Keys : unsigned short
	{
		KEY_SPACE		= 32,
		KEY_0           = 48,
		KEY_1           = 49,
		KEY_2           = 50,
		KEY_3           = 51,
		KEY_4           = 52,
		KEY_5           = 53,
		KEY_6           = 54,
		KEY_7           = 55,
		KEY_8           = 56,
		KEY_9           = 57,
		KEY_A			= 65,
		KEY_B			= 66,
		KEY_C			= 67,
		KEY_D			= 68,
		KEY_E			= 69,
		KEY_F			= 70,
		KEY_G			= 71,
		KEY_H			= 72,
		KEY_I			= 73,
		KEY_J			= 74,
		KEY_K			= 75,
		KEY_L			= 76,
		KEY_M			= 77,
		KEY_N			= 78,
		KEY_O			= 79,
		KEY_P			= 80,
		KEY_Q			= 81,
		KEY_R			= 82,
		KEY_S			= 83,
		KEY_T			= 84,
		KEY_U			= 85,
		KEY_V			= 86,
		KEY_W			= 87,
		KEY_X			= 88,
		KEY_Y			= 89,
		KEY_Z			= 90,
		KEY_ESCAPE		= 256,
		KEY_ENTER		= 257,
		KEY_TAB			= 258,
		KEY_BACKSPACE	= 259,
		KEY_DEL			= 261,
		KEY_F1          = 290,
		KEY_F2          = 291,
		KEY_F3          = 292,
		KEY_F4          = 293,
		KEY_F5          = 294,
		KEY_F6          = 295,
		KEY_F7          = 296,
		KEY_F8          = 297,
		KEY_F9          = 298,
		KEY_F10         = 299,
		KEY_F11         = 300,
		KEY_F12         = 301,
		KEY_LEFT_SHIFT	= 340,
		KEY_LEFT_CONTROL = 341,
	};

	struct Key
	{
		bool state;
		bool justReleased;
		bool justPressed;
	};

	struct MouseButton
	{
		bool state;
		bool justReleased;
		bool justPressed;
	};

	enum MouseButtonType
	{
		Left,
		Right
	};

	struct InputMapping
	{
		//unsigned int id;
		Keys positiveKey;
		Keys negativeKey;
		MouseButtonType mouseButton;
	};

	class FileManager;

	class InputManager
	{
	public:
		InputManager();

		void Update();			// Needs to be called at the start of the frame to reset the just released state.
		void Reset();

		// Tries to load the input mappings from file, if there is no file, it loads some default ones
		void LoadInputMappings(FileManager *fileManager, const std::string &path);

		int AnyKeyPressed() const;
		bool GetLastChar(unsigned char &c);
		Keys GetLastKeyPressed() const;
		bool IsKeyPressed(int keycode) const;
		bool WasKeyPressed(int keycode) const;
		bool WasKeyReleased(int keycode) const;
		bool WasMouseButtonReleased(int button);
		bool MouseMoved() const;
		bool IsMousePressed(int button) const;
		bool IsMouseButtonDown(int button) const;
		float GetScrollWheelY() const { return scrollWheelY; }
		float GetAxis(const std::string &name);
		bool GetAction(const std::string &name);

		std::unordered_map<std::string, InputMapping> &GetInputMappings() { return inputMappings; }
		const std::string &GetStringOfKey(Keys key) { return keysToString[key]; }

		void SetMousePosition(const glm::vec2 &pos);
		const glm::vec2 &GetMousePosition() const { return mousePosition; }

		void UpdateKeys(int key, int scancode, int action, int mods);
		void UpdateChar(unsigned char c);
		void SetMouseButtonState(int button, int action);
		void SetScrollWheelYOffset(float yoffset);

	private:
		Key keys[512];
		glm::vec2 mousePosition;
		bool mouseMoved;
		MouseButton mouseButtonsState[2];		// 0 is left mouse button, 1 is right mouse button
		unsigned char lastChar;
		Keys lastKeyPressed;
		bool charUpdated;
		float scrollWheelY;

		std::unordered_map<unsigned short, std::string> keysToString;
		std::unordered_map<std::string, InputMapping> inputMappings;
	};

	class Input
	{
	private:
		friend class InputManager;
	public:
		static int AnyKeyPressed() { return inputManager->AnyKeyPressed(); }
		static Keys GetLastKeyPressed() { return inputManager->GetLastKeyPressed(); }
		static bool GetLastChar(unsigned char &c) { return inputManager->GetLastChar(c); }
		static bool IsKeyPressed(int keycode) { return inputManager->IsKeyPressed(keycode); }
		static bool WasKeyPressed(int keycode) { return inputManager->WasKeyPressed(keycode); }
		static bool WasKeyReleased(int keycode) { return inputManager->WasKeyReleased(keycode); }
		static bool WasMouseButtonReleased(int button) { return inputManager->WasMouseButtonReleased(button); }
		static bool IsMousePressed(int button) { return inputManager->IsMousePressed(button); }
		static bool IsMouseButtonDown(int button) { return inputManager->IsMouseButtonDown(button); }
		static bool MouseMoved() { return inputManager->MouseMoved(); }
		static const glm::vec2 &GetMousePosition() { return inputManager->GetMousePosition(); }
		static float GetScrollWheelY() { return inputManager->GetScrollWheelY(); }
		static float GetAxis(const std::string &name) { return inputManager->GetAxis(name); }
		static bool GetAction(const std::string &name) { return inputManager->GetAction(name); }

		static InputManager *GetInputManager() { return inputManager; }

	private:
		static InputManager *inputManager;
	};
}
