#include "Widget.h"

#include "Game/Game.h"
#include "Game/Script.h"
#include "Program/Log.h"

#include "Button.h"

#include "Graphics/ResourcesLoader.h"

#include "include/glm/gtc/matrix_transform.hpp"

#include <iostream>

namespace Engine
{
	Widget::Widget(Game *game)
	{
		this->game = game;
		rect.position = glm::vec2();
		rect.size = glm::vec2();
		isEnabled = true;
		params.colorTint = glm::vec4(1.0f);
		params.depth = 0.0f;
	}

	Widget::~Widget()
	{
	}

	void Widget::Resize(unsigned int width, unsigned int height)
	{
		rect.position = glm::vec2(positionPercent.x / 100.0f * width, positionPercent.y / 100.0f * height);
	}

	void Widget::SetEnabled(bool enable)
	{
		isEnabled = enable;
	}

	void Widget::SetMaterial(MaterialInstance *matInstance)
	{
		this->matInstance = matInstance;
	}

	void Widget::SetRect(const Rect &rect)
	{
		this->rect = rect;
	}

	void Widget::SetRect(const glm::vec2 &pos, const glm::vec2 &size)
	{
		this->rect = { pos, size };
	}

	void Widget::SetRectPosPercent(const glm::vec2 &posPercent)
	{
		this->positionPercent = posPercent;
		unsigned int width = game->GetPlayableWidth();
		unsigned int height = game->GetPlayebleHeight();
		rect.position = glm::vec2(positionPercent.x / 100.0f * width, positionPercent.y / 100.0f * height);
	}

	void Widget::SetRectSize(const glm::vec2 &size)
	{
		rect.size = size;
	}

	const glm::mat4 &Widget::GetTransform()
	{
		transform = glm::translate(glm::mat4(1.0f), glm::vec3(rect.position, 0.0f));
		transform = glm::scale(transform, glm::vec3(rect.size * 0.5f, 1.0f));
		return transform;
	}

	void Widget::Serialize(Serializer &s)
	{
		s.Write(static_cast<int>(type));
		s.Write(isEnabled);
		s.Write(positionPercent);
		s.Write(rect.size);
		s.Write(params.colorTint);
		s.Write(params.depth);
	}

	void Widget::Deserialize(Serializer &s)
	{
		/*int temp = 0;
		s.Read(temp);
		type = static_cast<WidgetType>(temp);*/
		s.Read(isEnabled);
		s.Read(positionPercent);
		SetRectPosPercent(positionPercent);

		s.Read(rect.size);
		s.Read(params.colorTint);
		s.Read(params.depth);
	}

	Button *Widget::CastToButton(Widget *w)
	{
		if (!w)
			return nullptr;

		Button *b = static_cast<Button*>(w);
		return b;
	}
}
