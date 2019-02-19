#pragma once

#include "Widget.h"

namespace Engine
{
	class Texture;

	class Image : public Widget
	{
	private:
		friend class UIManager;
	public:
		Image(Game *game);
		~Image();

		void Update(float dt) override;
		void UpdateInGame(float dt) override;

		void SetTexture(Texture *texture);

		void Serialize(Serializer &s);
		void Deserialize(Serializer &s, Renderer *renderer);

		static Image *CastFromWidget(Widget *widget) { if (widget->GetType() == WidgetType::IMAGE) { return static_cast<Image*>(widget); } return nullptr; }

	private:
		Texture *texture;
	};
}
