#include "EditText.h"

#include "Game\Game.h"
#include "Graphics\Effects\MainView.h"
#include "Graphics\ResourcesLoader.h"
#include "Program\Input.h"

namespace Engine
{
	EditText::EditText(Game *game) : Widget(game)
	{
		type = WidgetType::EDIT_TEXT;
		rect.position = glm::vec2(0.0f);
		rect.size = glm::vec2(1.0f);
		text = "";
		textScale = glm::vec2(1.0f);

		backgroundTexture = nullptr;
	}

	EditText::~EditText()
	{
		if (backgroundTexture)
			backgroundTexture->RemoveReference();
	}

	void EditText::Update(float dt)
	{
		if (needsTextRecalc)
		{
			// If we have more text than available rect space then we need to clip the text to fit 
			glm::vec2 totalSize = glm::vec2();
			clippedText.clear();
			for (const char &c : text)
			{
				glm::vec2 size = game->GetRenderingPath()->GetFont().CalculateCharSize(c, textScale);
				totalSize += size;

				if (totalSize.x <= rect.size.x)
					clippedText.push_back(c);
			}
			needsTextRecalc = false;
		}

		game->GetRenderingPath()->GetFont().AddText(clippedText, glm::vec2(rect.position.x - rect.size.x * 0.5f, rect.position.y + rect.size.y * 0.5f), textScale);
	}

	void EditText::UpdateInGame(float dt)
	{
		Update(dt);

		if (Input::WasMouseButtonReleased(MouseButtonType::Left))
		{
			unsigned int height = game->GetRenderer()->GetHeight();

			const glm::vec2 &mousePos = Input::GetMousePosition();
			float mouseY = height - mousePos.y;

			glm::vec2 topLeft = rect.position - rect.size * 0.5f;
			glm::vec2 bottomRight = rect.position + rect.size * 0.5f;

			if (mousePos.x > topLeft.x && mousePos.x < bottomRight.x && mouseY > topLeft.y && mouseY < bottomRight.y)
				canEditText = true;
			else
				canEditText = false;
		}

		if (canEditText)
		{
			if (Input::WasKeyReleased(KEY_BACKSPACE))
			{
				if (text.length() > 0)
					text.pop_back();
				needsTextRecalc = true;
			}
			unsigned char c;
			if (Input::GetLastChar(c))
			{
				text.push_back(c);
				needsTextRecalc = true;
			}

			if (Input::WasKeyReleased(KEY_ENTER))
				canEditText = false;
		}
	}

	void EditText::SetText(const std::string &text)
	{
		this->text = text;
		needsTextRecalc = true;
	}

	void EditText::SetTextScale(const glm::vec2 &scale)
	{
		textScale = scale;
		needsTextRecalc = true;
	}

	void EditText::SetBackgroundTexture(Texture *texture)
	{
		if (!texture)
			return;

		if (backgroundTexture)
			backgroundTexture->RemoveReference();		// Remove ref of the previous texture

		backgroundTexture = texture;
		matInstance->textures[0] = backgroundTexture;
		game->GetRenderer()->UpdateMaterialInstance(matInstance);
	}

	void EditText::SetRectSize(const glm::vec2 &size)
	{
		Widget::SetRectSize(size);
		needsTextRecalc = true;
	}

	void EditText::Serialize(Serializer &s)
	{
		Widget::Serialize(s);
		s.Write(text);
		s.Write(textScale);

		if (backgroundTexture)
		{
			s.Write(true);
			s.Write(backgroundTexture->GetPath());
		}
		else
			s.Write(false);
	}

	void EditText::Deserialize(Serializer &s, Renderer *renderer)
	{
		Widget::Deserialize(s);
		s.Read(text);
		s.Read(textScale);
		needsTextRecalc = true;

		TextureParams p = { TextureWrap::REPEAT, TextureFilter::LINEAR, TextureFormat::RGBA, TextureInternalFormat::SRGB8_ALPHA8, TextureDataType::UNSIGNED_BYTE, true, false };

		bool hasBackground = false;
		s.Read(hasBackground);
		if (hasBackground)
		{
			std::string path;
			s.Read(path);
			backgroundTexture = renderer->CreateTexture2D(path, p);
		}
		else
		{
			backgroundTexture = renderer->CreateTexture2D("Data/Resources/Textures/white.dds", p);
		}

		matInstance->textures[0] = backgroundTexture;
	}
}
