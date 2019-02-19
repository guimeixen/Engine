#include "Button.h"

#include "Program\Input.h"
#include "Graphics\Renderer.h"
#include "Graphics\ResourcesLoader.h"
#include "Graphics\Material.h"
#include "Graphics\Effects\MainView.h"

#include "Game\Game.h"
#include "Game\Script.h"

#include <iostream>

namespace Engine
{
	Button::Button(Game *game) : Widget(game)
	{
		type = WidgetType::BUTTON;
		rect.position = glm::vec2(0.0f);
		rect.size = glm::vec2(1.0f);
		text = "";
		textScale = glm::vec2(1.0f);
		textColor = glm::vec4(1.0f);
		textPosOffset = glm::vec2(0.0f);

		idleTexture = nullptr;
		hoverTexture = nullptr;
		pressedTexture = nullptr;
	}

	Button::~Button()
	{
		if (idleTexture)
			idleTexture->RemoveReference();
		if (hoverTexture)
			hoverTexture->RemoveReference();
		if (pressedTexture)
			pressedTexture->RemoveReference();
	}

	void Button::Update(float dt)
	{
		if (needsTextSizeRecalc)
		{
			rect.size = game->GetRenderingPath()->GetFont().CalculateTextSize(text, textScale);
			needsTextSizeRecalc = false;
		}

		game->GetRenderingPath()->GetFont().AddText(text, glm::vec2(rect.position.x - rect.size.x * 0.5f + textPosOffset.x, rect.position.y + rect.size.y * 0.5f + textPosOffset.y), textScale, textColor);
	}

	void Button::UpdateInGame(float dt)
	{
		CheckButtonPressed();

		Update(dt);
	}

	void Button::CheckButtonPressed()
	{
		isPressed = false;
		unsigned int height = game->GetRenderer()->GetHeight();

		const glm::vec2 &mousePos = Input::GetMousePosition();
		float mouseY = height - mousePos.y;

		glm::vec2 topLeft = rect.position - rect.size * 0.5f;
		glm::vec2 bottomRight = rect.position + rect.size * 0.5f;

		if (mousePos.x > topLeft.x && mousePos.x < bottomRight.x && mouseY > topLeft.y && mouseY < bottomRight.y)
		{
			matInstance->textures[0] = hoverTexture;
			//game->GetRenderer()->UpdateMaterialInstance(matInstance);				// Only update when the first time

			if (Input::IsMouseButtonDown(MouseButtonType::Left))
			{
				matInstance->textures[0] = pressedTexture;
				//game->GetRenderer()->UpdateMaterialInstance(matInstance);		
			}
			if (Input::WasMouseButtonReleased(MouseButtonType::Left))
			{
				// Replace with entity
				/*if (script)
					script->CallOnButtonPressed();*/

				isPressed = true;
			}
		}
		else
		{
			matInstance->textures[0] = idleTexture;
			//game->GetRenderer()->UpdateMaterialInstance(matInstance);
		}
	}

	void Button::SetText(const std::string &text)
	{
		this->text = text;
		needsTextSizeRecalc = true;
	}

	void Button::SetTextScale(const glm::vec2 &scale)
	{
		textScale = scale;
	}

	void Button::SetTextColor(const glm::vec4 &color)
	{
		textColor = color;
	}

	void Button::SetTextPosOffset(const glm::vec2 &offset)
	{
		textPosOffset = offset;
	}

	void Button::SetIdleTexture(Texture *texture)
	{
		if (!texture)
			return;

		if (idleTexture)
			idleTexture->RemoveReference();		// Remove ref of the previous texture

		idleTexture = texture;
		matInstance->textures[0] = idleTexture;			// Set the texture to the idle texture so it displays properly in the editor
		game->GetRenderer()->UpdateMaterialInstance(matInstance);
	}

	void Button::SetHoverTexture(Texture *texture)
	{
		if (!texture)
			return;

		if (hoverTexture)
			hoverTexture->RemoveReference();		// Remove ref of the previous texture

		hoverTexture = texture;
	}

	void Button::SetPressedTexture(Texture *texture)
	{
		if (!texture)
			return;

		if (pressedTexture)
			pressedTexture->RemoveReference();		// Remove ref of the previous texture

		pressedTexture = texture;
	}

	void Button::Serialize(Serializer &s)
	{
		Widget::Serialize(s);
		s.Write(text);
		s.Write(textScale);
		s.Write(textPosOffset);
		if (idleTexture)
		{
			s.Write(true);
			s.Write(idleTexture->GetPath());
		}
		else
			s.Write(false);

		if (hoverTexture)
		{
			s.Write(true);
			s.Write(hoverTexture->GetPath());
		}
		else
			s.Write(false);

		if (pressedTexture)
		{
			s.Write(true);
			s.Write(pressedTexture->GetPath());
		}
		else
			s.Write(false);
	}

	void Button::Deserialize(Serializer &s, Renderer *renderer)
	{
		Widget::Deserialize(s);
		s.Read(text);
		s.Read(textScale);
		s.Read(textPosOffset);

		TextureParams p = { TextureWrap::REPEAT, TextureFilter::LINEAR, TextureFormat::RGBA, TextureInternalFormat::SRGB8_ALPHA8, TextureDataType::UNSIGNED_BYTE, true, false };

		bool hasIdle = false;
		s.Read(hasIdle);
		if (hasIdle)
		{
			std::string path;
			s.Read(path);
			idleTexture = renderer->CreateTexture2D(path, p);
		}
		else
		{
			idleTexture = renderer->CreateTexture2D("Data/Resources/Textures/white.dds", p);
		}

		bool hasHover = false;
		s.Read(hasHover);
		if (hasHover)
		{
			std::string path;
			s.Read(path);
			hoverTexture = renderer->CreateTexture2D(path, p);
		}
		else
		{
			hoverTexture = renderer->CreateTexture2D("Data/Resources/Textures/light_gray.dds", p);
		}

		bool hasPressed = false;
		s.Read(hasPressed);
		if (hasPressed)
		{
			std::string path;
			s.Read(path);
			pressedTexture = renderer->CreateTexture2D(path, p);
		}
		else
		{
			pressedTexture = renderer->CreateTexture2D("Data/Resources/Textures/gray.dds", p);
		}


		matInstance->textures[0] = idleTexture;
		game->GetRenderer()->UpdateMaterialInstance(matInstance);
	}
}
