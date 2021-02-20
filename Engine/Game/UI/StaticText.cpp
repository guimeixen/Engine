#include "StaticText.h"

#include "Game/Game.h"

#include <iostream>

namespace Engine
{
	StaticText::StaticText(Game *game) : Widget(game)
	{
		type = WidgetType::TEXT;
		textScale = glm::vec2(1.0f);
	}

	StaticText::~StaticText()
	{
	}

	void StaticText::Update(float dt)
	{
		if (needsTextSizeRecalc)
		{
			rect.size = game->GetRenderingPath()->GetFont().CalculateTextSize(text, textScale);
			needsTextSizeRecalc = false;
		}

		game->GetRenderingPath()->GetFont().AddText(text, glm::vec2(rect.position.x - rect.size.x * 0.5f, rect.position.y + rect.size.y * 0.5f), textScale, params.colorTint);
	}

	void StaticText::UpdateInGame(float dt)
	{
		Update(dt);
	}

	void StaticText::SetText(const std::string &text)
	{
		this->text = text;
		needsTextSizeRecalc = true;
	}

	void StaticText::Serialize(Serializer &s)
	{
		Widget::Serialize(s);
		s.Write(text);
		s.Write(textScale);
	}

	void StaticText::Deserialize(Serializer &s)
	{
		Widget::Deserialize(s);
		s.Read(text);
		s.Read(textScale);
		needsTextSizeRecalc = true;
	}
}
