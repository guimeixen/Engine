#pragma once

#include "Widget.h"

#include <string>

namespace Engine
{
	class Texture;

	class Button : public Widget
	{
	private:
		friend class UIManager;
	public:
		Button(Game *game);
		~Button();

		void Update(float dt) override;
		void UpdateInGame(float dt) override;

		void CheckButtonPressed();

		bool IsPressed()const { return isPressed; }

		void SetText(const std::string &text);
		void SetTextScale(const glm::vec2 &scale);
		void SetTextColor(const glm::vec4 &color);
		void SetTextPosOffset(const glm::vec2 &offset);
		void SetIdleTexture(Texture *texture);
		void SetHoverTexture(Texture *texture);
		void SetPressedTexture(Texture *texture);

		const std::string &GetText() const { return text; }
		glm::vec2 GetTextScale() const { return textScale; }
		const glm::vec4 &GetTextColor() const { return textColor; }
		const glm::vec2 &GetTextPosOffset() const { return textPosOffset; }

		void Serialize(Serializer &s);
		void Deserialize(Serializer &s, Renderer *renderer);

		static Button *CastFromWidget(Widget *widget) { if (widget->GetType() == WidgetType::BUTTON) { return static_cast<Button*>(widget); } return nullptr; }

	private:
		bool hovered = false;
		bool pressed = false;
		Texture *idleTexture;
		Texture *hoverTexture;
		Texture *pressedTexture;
		std::string text;
		glm::vec2 textScale;
		glm::vec2 textPosition;
		glm::vec4 textColor;
		glm::vec2 textPosOffset;
		//glm::vec2 textSize;
		bool needsTextSizeRecalc = false;
		bool isPressed = false;
	};
}
