#pragma once

#include "include\glm\glm.hpp"

namespace Engine
{
#define KEY_SPACE		32
#define KEY_0           48
#define KEY_1           49
#define KEY_2           50
#define KEY_3           51
#define KEY_4           52
#define KEY_5           53
#define KEY_6           54
#define KEY_7           55
#define KEY_8           56
#define KEY_9           57
#define KEY_A			65
#define KEY_B			66
#define KEY_C			67
#define KEY_D			68
#define KEY_E			69
#define KEY_F			70
#define KEY_G			71
#define KEY_H			72
#define KEY_I			73
#define KEY_J			74
#define KEY_K			75
#define KEY_L			76
#define KEY_M			77
#define KEY_N			78
#define KEY_O			79
#define KEY_P			80
#define KEY_Q			81
#define KEY_R			82
#define KEY_S			83
#define KEY_T			84
#define KEY_U			85
#define KEY_V			86
#define KEY_W			87
#define KEY_X			88
#define KEY_Y			89
#define KEY_Z			90
#define KEY_ESCAPE		256
#define KEY_ENTER		257
#define KEY_TAB			258
#define KEY_BACKSPACE	259
#define KEY_DEL			261
#define KEY_LEFT_SHIFT	340
#define KEY_LEFT_CONTROL 341

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

	class InputManager
	{
	public:
		InputManager();
		~InputManager();

		void Update();			// Needs to be called at the start of the frame to reset the just released state. If two requests of WasKeyReleased() are made to the same key only the first would return
								// the correct state because in the first call it would then reset the state.
		void Reset();

		int AnyKeyPressed() const;
		bool GetLastChar(unsigned char &c);
		bool IsKeyPressed(int keycode) const;
		bool WasKeyPressed(int keycode) const;
		bool WasKeyReleased(int keycode) const;
		bool WasMouseButtonReleased(int button);
		bool MouseMoved() const;
		bool IsMousePressed(int button) const;
		bool IsMouseButtonDown(int button) const;
		float GetScrollWheelY() const { return scrollWheelY; }

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
		MouseButton mouseButtonsState[2];		// 0 if left mouse button, 1 is right mouse button
		unsigned char lastChar;
		bool charUpdated;
		float scrollWheelY;
	};

	class Input
	{
	private:
		friend class InputManager;
	public:
		static int AnyKeyPressed() { return inputManager->AnyKeyPressed(); }
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

		static InputManager *GetInputManager() { return inputManager; }

	private:
		static InputManager *inputManager;
	};
}
