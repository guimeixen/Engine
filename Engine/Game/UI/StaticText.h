#pragma once

#include "Widget.h"

#include <string>

namespace Engine
{
	class StaticText : public Widget
	{
	private:
		friend class UIManager;
	public:
		StaticText(Game *game);
		~StaticText();

		void Update(float dt) override;
		void UpdateInGame(float dt) override;

		void SetText(const std::string &text);
		void SetTextScale(const glm::vec2 &textScale) { this->textScale = textScale; }

		const std::string &GetText() const { return text; }
		const glm::vec2 &GetTextScale() const { return textScale; }

		void Serialize(Serializer &s);
		void Deserialize(Serializer &s);

		static StaticText *CastFromWidget(Widget *widget) { if (widget->GetType() == WidgetType::TEXT) { return static_cast<StaticText*>(widget); } return nullptr; }

	private:
		std::string text;
		glm::vec2 textScale;
		bool needsTextSizeRecalc = false;
	};
}
