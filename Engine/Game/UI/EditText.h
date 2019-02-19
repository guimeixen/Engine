#pragma once

#include "Widget.h"

namespace Engine
{
	class Texture;

	class EditText : public Widget
	{
	private:
		friend class UIManager;
	public:
		EditText(Game *game);
		~EditText();

		void Update(float dt) override;
		void UpdateInGame(float dt) override;

		void SetText(const std::string &text);
		void SetTextScale(const glm::vec2 &scale);
		void SetBackgroundTexture(Texture *texture);
		void SetRectSize(const glm::vec2 &size);

		const std::string &GetText() const { return text; }
		glm::vec2 GetTextScale() const { return textScale; }

		void Serialize(Serializer &s);
		void Deserialize(Serializer &s, Renderer *renderer);

		static EditText *CastFromWidget(Widget *widget) { if (widget->GetType() == WidgetType::EDIT_TEXT) { return static_cast<EditText*>(widget); } return nullptr; }

	private:
		std::string text;
		std::string clippedText;
		glm::vec2 textScale;
		bool needsTextRecalc = false;
		bool canEditText = false;
		Texture *backgroundTexture;
	};
}
