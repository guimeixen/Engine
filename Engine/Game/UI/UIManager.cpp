#include "UIManager.h"

#include "Game\Game.h"

#include "Graphics\Renderer.h"
#include "Graphics\ResourcesLoader.h"
#include "Graphics\Material.h"
#include "Graphics\MeshDefaults.h"
#include "Graphics\Effects\MainView.h"
#include "Graphics\VertexArray.h"

#include "StaticText.h"
#include "Button.h"
#include "Image.h"
#include "EditText.h"

#include "Program\Input.h"
#include "Program\Utils.h"
#include "Program\StringID.h"

#include "include\glm\gtc\matrix_transform.hpp"

#include <iostream>
#include <ostream>

namespace Engine
{
	void UIManager::Init(Game *game)
	{
		this->game = game;
		mesh = MeshDefaults::CreateQuad(game->GetRenderer());

		showCursor = true;
		cursor = nullptr;
		//FIX IDS
		/*cursor = new Image(GetNextID(), game);
		cursor->SetRectPosPercent(glm::vec2(50.0f, 50.0f));
		TextureParams p = { TextureWrap::REPEAT, TextureFilter::LINEAR, TextureFormat::RGBA, TextureInternalFormat::SRGB8_ALPHA8, TextureDataType::UNSIGNED_BYTE, true, false };
		cursor->SetTexture(ResourcesLoader::LoadTexture2D("Data/Resources/Textures/cursor.png", p));
		cursor->SetRectSize(glm::vec2(16.0f, 26.0f));
		cursor->SetColorTintRGBA(glm::vec4(1.0f));
		cursor->SetDepth(0.0f);*/
	}

	void UIManager::PartialDispose()
	{
		for (unsigned int i = 0; i < usedWidgets; i++)
		{
			if (widgets[i].w)
				delete widgets[i].w;
		}
		widgets.clear();
	}

	void UIManager::Dispose()
	{
		PartialDispose();

		if (mesh.vao)
			delete mesh.vao;

		if (cursor)
			delete cursor;	
	}

	void UIManager::Update(float dt)
	{
		for (unsigned int i = 0; i < usedWidgets; i++)
		{
			if (widgets[i].w->IsEnabled())
				widgets[i].w->Update(dt);
		}
	}

	void UIManager::UpdateInGame(float dt)
	{
#ifndef EDITOR
		if (!cursor)
		{
			cursor = new Image(game);
			cursor->SetRectPosPercent(glm::vec2(50.0f, 50.0f));
			cursor->SetMaterial(game->GetRenderer()->CreateMaterialInstance(game->GetScriptManager(), "Data/Resources/Materials/ui.mat", mesh.vao->GetVertexInputDescs()));
			TextureParams p = { TextureWrap::REPEAT, TextureFilter::LINEAR, TextureFormat::RGBA, TextureInternalFormat::SRGB8_ALPHA8, TextureDataType::UNSIGNED_BYTE, true, false };
			cursor->SetTexture(game->GetRenderer()->CreateTexture2D("Data/Resources/Textures/cursor.png", p));
			cursor->SetRectSize(glm::vec2(16.0f, 26.0f));
			cursor->SetColorTintRGBA(glm::vec4(1.0f));
			cursor->SetDepth(-0.05f);
		}
		if (Input::MouseMoved())
		{
			//const glm::vec2 &mousePos = Input::GetMousePosition();
			cursorPos = Input::GetMousePosition();
			glm::vec2 posPercent = glm::vec2(cursorPos.x / game->GetRenderer()->GetWidth(), cursorPos.y / game->GetRenderer()->GetHeight()) * 100.0f;
			posPercent.y = 100.0f - posPercent.y;

			if (posPercent.x < 0.0f)
				posPercent.x = 0.0f;
			else if (posPercent.x > 100.0f)
				posPercent.x = 100.0f;

			if (posPercent.y < 0.0f)
				posPercent.y = 0.0f;
			else if (posPercent.y > 100.0f)
				posPercent.y = 100.0f;

			cursor->SetRectPosPercent(posPercent);
		}
#endif

		for (unsigned int i = 0; i < usedWidgets; i++)
		{
			if (widgets[i].w->IsEnabled())
				widgets[i].w->UpdateInGame(dt);
		}
	}

	void UIManager::Resize(unsigned int width, unsigned int height)
	{
		if (cursor)
		{
			cursor->SetRectPosPercent(glm::vec2(50.0f, 50.0f));
			cursorPos.x = (float)width / 2.0f;
			cursorPos.y = (float)height / 2.0f;
		}

		for (size_t i = 0; i < usedWidgets; i++)
		{
			widgets[i].w->Resize(width, height);
		}
	}

	void UIManager::Cull(unsigned int passAndFrustumCount, unsigned int *passIds, const Frustum *frustums, std::vector<VisibilityIndices*> &out)
	{
		if (usedWidgets <= 0)
			return;

		for (unsigned int i = 0; i < passAndFrustumCount; i++)
		{
			out[i]->push_back(0);			// Push back at least one index per frustum so GetRenderItems gets called
		}
	}

	void UIManager::GetRenderItems(unsigned int passCount, unsigned int *passIds, const VisibilityIndices &visibility, RenderQueue &outQueues)
	{
		const ShaderPass &pass = widgets[0].w->matInstance->baseMaterial->GetShaderPass(0);			// We can use the first widget's shader pass because all widgets use the same material

		for (size_t i = 0; i < passCount; i++)
		{
			if (passIds[i] == pass.queueID)
			{
				for (unsigned int j = 0; j < usedWidgets; j++)
				{
					Widget *widget = widgets[j].w;
					WidgetType type = widget->GetType();

					if (type != WidgetType::TEXT && widget->IsEnabled())		// Text is rendered separately
					{
						RenderItem ri = {};
						ri.mesh = &mesh;
						ri.matInstance = widget->matInstance;
						ri.shaderPass = 0;
						ri.transform = &widget->GetTransform();
						ri.materialData = &widget->GetParams();
						ri.materialDataSize = sizeof(WidgetShaderParams);

						//outQueues[i].push_back(ri);
						outQueues.push_back(ri);
					}
				}

#ifndef EDITOR
				// Always push the cursor
				/*if (showCursor)
				{
					RenderItem ri = {};
					ri.mesh = &mesh;
					ri.matInstance = cursor->matInstance;
					ri.shaderPass = 0;				// Pass 0. Get ID of pass instead of hardcoded
					const Rect &rect = cursor->GetRect();
					ri.transform = glm::translate(glm::mat4(1.0f), glm::vec3(rect.position.x + rect.size.x * 0.5f, rect.position.y - rect.size.y * 0.5f, 0.0f));		// Put the mouse on top left
					ri.transform = glm::scale(ri.transform, glm::vec3(rect.size * 0.5f, 1.0f));
					ri.meshParams = &cursor->GetParams();
					ri.meshParamsSize = sizeof(WidgetShaderParams);

					outQueues[i].push_back(ri);
				}*/
#endif
			}
		}
	}

	Widget *UIManager::AddButton(Entity e)
	{
		if (HasWidget(e))
			return GetWidget(e);	

		Button *button = new Button(game);
		button->SetRectPosPercent(glm::vec2(50.0f, 50.0f));
		button->SetText("Button");
		button->SetRectSize(game->GetRenderingPath()->GetFont().CalculateTextSize("Button", glm::vec2(1.0f)));

		button->SetMaterial(game->GetRenderer()->CreateMaterialInstance(game->GetScriptManager(), "Data/Resources/Materials/ui.mat", mesh.vao->GetVertexInputDescs()));

		TextureParams p = { TextureWrap::REPEAT, TextureFilter::LINEAR, TextureFormat::RGBA, TextureInternalFormat::SRGB8_ALPHA8, TextureDataType::UNSIGNED_BYTE, true, false };

		button->SetIdleTexture(game->GetRenderer()->CreateTexture2D("Data/Resources/Textures/white.dds", p));
		button->SetHoverTexture(game->GetRenderer()->CreateTexture2D("Data/Resources/Textures/light_gray.dds", p));
		button->SetPressedTexture(game->GetRenderer()->CreateTexture2D("Data/Resources/Textures/gray.dds", p));

		WidgetInstance wi;
		wi.e = e;
		wi.w = button;

		if (usedWidgets < widgets.size())
		{
			widgets[usedWidgets] = wi;
			map[e.id] = usedWidgets;
		}
		else
		{
			widgets.push_back(wi);
			map[e.id] = (unsigned int)widgets.size() - 1;
		}

		usedWidgets++;

		return button;
	}

	Widget *UIManager::AddText(Entity e)
	{
		if (HasWidget(e))
			return GetWidget(e);

		StaticText *text = new StaticText(game);
		text->SetRectPosPercent(glm::vec2(50.0f, 50.0f));
		text->SetText("Text");
		text->SetRectSize(game->GetRenderingPath()->GetFont().CalculateTextSize("Text", glm::vec2(1.0f)));
		text->SetMaterial(game->GetRenderer()->CreateMaterialInstance(game->GetScriptManager(), "Data/Resources/Materials/ui.mat", mesh.vao->GetVertexInputDescs()));
		
		WidgetInstance wi;
		wi.e = e;
		wi.w = text;

		if (usedWidgets < widgets.size())
		{
			widgets[usedWidgets] = wi;
			map[e.id] = usedWidgets;
		}
		else
		{
			widgets.push_back(wi);
			map[e.id] = (unsigned int)widgets.size() - 1;
		}

		usedWidgets++;

		return text;
	}

	Widget *UIManager::AddImage(Entity e)
	{
		if (HasWidget(e))
			return GetWidget(e);

		Image *image = new Image(game);
		image->SetRectPosPercent(glm::vec2(50.0f, 50.0f));
		TextureParams p = { TextureWrap::REPEAT, TextureFilter::LINEAR, TextureFormat::RGBA, TextureInternalFormat::SRGB8_ALPHA8, TextureDataType::UNSIGNED_BYTE, true, false };	
		image->SetRectSize(glm::vec2(100.0f, 100.0f));
		image->SetMaterial(game->GetRenderer()->CreateMaterialInstance(game->GetScriptManager(), "Data/Resources/Materials/ui.mat", mesh.vao->GetVertexInputDescs()));
		image->SetTexture(game->GetRenderer()->CreateTexture2D("Data/Resources/Textures/white.dds", p));
	
		WidgetInstance wi;
		wi.e = e;
		wi.w = image;

		if (usedWidgets < widgets.size())
		{
			widgets[usedWidgets] = wi;
			map[e.id] = usedWidgets;
		}
		else
		{
			widgets.push_back(wi);
			map[e.id] = (unsigned int)widgets.size() - 1;
		}

		usedWidgets++;

		return image;
	}

	Widget *UIManager::AddEditText(Entity e)
	{
		if (HasWidget(e))
			return GetWidget(e);

		EditText *editText = new EditText(game);
		editText->SetRectPosPercent(glm::vec2(50.0f, 50.0f));
		editText->SetText("Edit text");
		editText->SetRectSize(game->GetRenderingPath()->GetFont().CalculateTextSize("Edit text", glm::vec2(1.0f)));
		editText->SetMaterial(game->GetRenderer()->CreateMaterialInstance(game->GetScriptManager(), "Data/Resources/Materials/ui.mat", mesh.vao->GetVertexInputDescs()));

		TextureParams p = { TextureWrap::REPEAT, TextureFilter::LINEAR, TextureFormat::RGBA, TextureInternalFormat::SRGB8_ALPHA8, TextureDataType::UNSIGNED_BYTE, true, false };

		editText->SetBackgroundTexture(game->GetRenderer()->CreateTexture2D("Data/Resources/Textures/white.dds", p));

		WidgetInstance wi;
		wi.e = e;
		wi.w = editText;

		if (usedWidgets < widgets.size())
		{
			widgets[usedWidgets] = wi;
			map[e.id] = usedWidgets;
		}
		else
		{
			widgets.push_back(wi);
			map[e.id] = (unsigned int)widgets.size() - 1;
		}

		usedWidgets++;

		return editText;
	}

	void UIManager::DuplicateWidget(Entity e, Entity newE)
	{
		if (HasWidget(e) == false)
			return;

		const Widget *w = GetWidget(e);

		Widget *newW = nullptr;

		if (w->GetType() == WidgetType::BUTTON)
		{
			newW = new Button(game);
			const Button *b = static_cast<const Button*>(w);
			Button *newB = static_cast<Button*>(newW);

			newB->SetMaterial(game->GetRenderer()->CreateMaterialInstance(game->GetScriptManager(), b->matInstance->path, mesh.vao->GetVertexInputDescs()));
			newB->SetIdleTexture(b->idleTexture);
			newB->SetHoverTexture(b->hoverTexture);
			newB->SetPressedTexture(b->pressedTexture);

			b->idleTexture->AddReference();					// Add ref because we're using the texture again on another button
			b->hoverTexture->AddReference();
			b->pressedTexture->AddReference();

			newB->text = b->text;
			newB->textScale = b->textScale;
			newB->textPosition = b->textPosition;
			newB->textColor = b->textColor;
			newB->textPosOffset = b->textPosOffset;
		}
		else if (w->GetType() == WidgetType::TEXT)
		{
			newW = new StaticText(game);
			const StaticText *st = static_cast<const StaticText*>(w);
			StaticText *newST = static_cast<StaticText*>(newW);

			newST->SetMaterial(game->GetRenderer()->CreateMaterialInstance(game->GetScriptManager(), st->matInstance->path, mesh.vao->GetVertexInputDescs()));
			newST->text = st->text;
			newST->textScale = st->textScale;
		}
		else if (w->GetType() == WidgetType::IMAGE)
		{
			newW = new Image(game);
			const Image *i = static_cast<const Image*>(w);
			Image *newI = static_cast<Image*>(newW);

			newI->SetMaterial(game->GetRenderer()->CreateMaterialInstance(game->GetScriptManager(), i->matInstance->path, mesh.vao->GetVertexInputDescs()));
			newI->SetTexture(i->texture);
			i->texture->AddReference();
		}
		else if (w->GetType() == WidgetType::EDIT_TEXT)
		{
			newW = new EditText(game);
			const EditText *et = static_cast<const EditText*>(w);
			EditText *newET = static_cast<EditText*>(newW);

			newET->SetMaterial(game->GetRenderer()->CreateMaterialInstance(game->GetScriptManager(), et->matInstance->path, mesh.vao->GetVertexInputDescs()));
			newET->SetBackgroundTexture(et->backgroundTexture);
			et->backgroundTexture->AddReference();

			newET->text = et->text;
			newET->clippedText = et->clippedText;
			newET->textScale = et->textScale;
		}

		if (!newW)
			return;

		newW->isEnabled = w->isEnabled;
		newW->positionPercent = w->positionPercent;
		newW->rect = w->rect;
		newW->params.colorTint = w->params.colorTint;
		newW->params.depth = w->params.depth;

		WidgetInstance wi;
		wi.e = newE;
		wi.w = newW;

		if (usedWidgets < widgets.size())
		{
			widgets[usedWidgets] = wi;
			map[newE.id] = usedWidgets;
		}
		else
		{
			widgets.push_back(wi);
			map[newE.id] = (unsigned int)widgets.size() - 1;
		}

		usedWidgets++;
	}

	void UIManager::RemoveWidget(Entity e)
	{
		if (HasWidget(e))
		{
			unsigned int index = map.at(e.id);

			WidgetInstance temp = widgets[index];
			WidgetInstance last = widgets[widgets.size() - 1];
			widgets[widgets.size() - 1] = temp;
			widgets[index] = last;

			map[last.e.id] = index;
			map.erase(e.id);

			delete temp.w;
			usedWidgets--;
		}
	}

	bool UIManager::HasWidget(Entity e) const
	{
		return map.find(e.id) != map.end();
	}

	Widget *UIManager::GetWidget(Entity e) const
	{
		return widgets[map.at(e.id)].w;
	}

	Entity UIManager::PerformRaycast(const glm::vec2 &point)
	{
		for (unsigned int i = 0; i < usedWidgets; i++)
		{
			const Rect &rect = widgets[i].w->GetRect();
			glm::vec2 topLeft = rect.position - rect.size * 0.5f;
			glm::vec2 bottomRight = rect.position + rect.size * 0.5f;

			if (point.x > topLeft.x && point.x < bottomRight.x && point.y > topLeft.y && point.y < bottomRight.y)
			{
				return widgets[i].e;
			}
		}

		return { std::numeric_limits<unsigned int>::max() };
	}

	void UIManager::Serialize(Serializer &s) const
	{
		s.Write(usedWidgets);
		for (unsigned int i = 0; i < usedWidgets; i++)
		{
			const WidgetInstance &wi = widgets[i];
			s.Write(wi.e.id);
			widgets[i].w->Serialize(s);
		}
	}

	void UIManager::Deserialize(Serializer &s, bool reload)
	{
		Renderer *renderer = game->GetRenderer();

		if (!reload)
		{
			s.Read(usedWidgets);
			widgets.resize(usedWidgets);
			for (unsigned int i = 0; i < usedWidgets; i++)
			{
				WidgetInstance wi;
				s.Read(wi.e.id);

				int type = 0;
				s.Read(type);
				if (type == 0)
				{
					StaticText *text = new StaticText(game);
					text->SetMaterial(renderer->CreateMaterialInstance(game->GetScriptManager(), "Data/Resources/Materials/ui.mat", mesh.vao->GetVertexInputDescs()));
					text->Deserialize(s);

					wi.w = text;
				}
				else if (type == 1)
				{
					Button *button = new Button(game);
					button->SetMaterial(renderer->CreateMaterialInstance(game->GetScriptManager(), "Data/Resources/Materials/ui.mat", mesh.vao->GetVertexInputDescs()));
					button->Deserialize(s, renderer);

					wi.w = button;
				}
				else if (type == 2)
				{
					Image *image = new Image(game);
					image->SetMaterial(renderer->CreateMaterialInstance(game->GetScriptManager(), "Data/Resources/Materials/ui.mat", mesh.vao->GetVertexInputDescs()));
					image->Deserialize(s, renderer);

					wi.w = image;
				}
				else if (type == 3)
				{
					EditText *editText = new EditText(game);
					editText->SetMaterial(renderer->CreateMaterialInstance(game->GetScriptManager(), "Data/Resources/Materials/ui.mat", mesh.vao->GetVertexInputDescs()));
					editText->Deserialize(s, renderer);

					wi.w = editText;
				}

				widgets[i] = wi;
				map[wi.e.id] = i;
			}
		}
		else
		{
			s.Read(usedWidgets);
			for (unsigned int i = 0; i < usedWidgets; i++)
			{
				WidgetInstance &wi = widgets[i];
				s.Read(wi.e.id);

				int type = 0;
				s.Read(type);
				wi.w->Deserialize(s);

				/*int type = 0;
				s.Read(type);
				if (type == 0)
				{
					text->SetMaterial(renderer->CreateMaterialInstance(game->GetScriptManager(), "Data/Resources/Materials/ui.mat", mesh.vao->GetVertexInputDescs()));
					text->Deserialize(s);
				}
				else if (type == 1)
				{
					button->SetMaterial(renderer->CreateMaterialInstance(game->GetScriptManager(), "Data/Resources/Materials/ui.mat", mesh.vao->GetVertexInputDescs()));
					button->Deserialize(s, renderer);
				}
				else if (type == 2)
				{
					image->SetMaterial(renderer->CreateMaterialInstance(game->GetScriptManager(), "Data/Resources/Materials/ui.mat", mesh.vao->GetVertexInputDescs()));
					image->Deserialize(s, renderer);
				}
				else if (type == 3)
				{
					editText->SetMaterial(renderer->CreateMaterialInstance(game->GetScriptManager(), "Data/Resources/Materials/ui.mat", mesh.vao->GetVertexInputDescs()));
					editText->Deserialize(s, renderer);
				}*/
			}
		}
	}
}
