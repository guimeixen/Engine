#include "Image.h"

#include "Game/Game.h"
#include "Graphics/Texture.h"
#include "Graphics/Material.h"

namespace Engine
{
	Image::Image(Game *game) : Widget(game)
	{
		type = WidgetType::IMAGE;
		rect.position = glm::vec2(0.0f);
		rect.size = glm::vec2(1.0f);
		texture = nullptr;
	}

	Image::~Image()
	{
		if (texture)
			texture->RemoveReference();
	}

	void Image::Update(float dt)
	{
	}

	void Image::UpdateInGame(float dt)
	{
	}

	void Image::SetTexture(Texture *texture)
	{
		if (!texture)
			return;

		if (this->texture)
			this->texture->RemoveReference();		// Remove ref of the previous texture

		this->texture = texture;
		matInstance->textures[0] = texture;
		game->GetRenderer()->UpdateMaterialInstance(matInstance);
	}

	void Image::Serialize(Serializer &s)
	{
		Widget::Serialize(s);
		s.Write(texture->GetPath());
	}

	void Image::Deserialize(Serializer &s, Renderer *renderer)
	{
		Widget::Deserialize(s);
		std::string path;
		s.Read(path);

		TextureParams p = { TextureWrap::REPEAT, TextureFilter::LINEAR, TextureFormat::RGBA, TextureInternalFormat::SRGB8_ALPHA8, TextureDataType::UNSIGNED_BYTE, true, false };
		texture = renderer->CreateTexture2D(path, p);
		matInstance->textures[0] = texture;
	}
}
