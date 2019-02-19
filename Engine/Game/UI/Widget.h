#pragma once

#include "Program\Serializer.h"

#include "include\glm\glm.hpp"

namespace Engine
{
	struct Rect
	{
		glm::vec2 position;
		glm::vec2 size;
	};

	enum class WidgetType
	{
		TEXT,
		BUTTON,
		IMAGE,
		EDIT_TEXT
	};

	struct WidgetShaderParams
	{
		glm::vec4 colorTint;
		float depth;
	};

	class Renderer;
	class Game;
	struct MaterialInstance;
	class Button;

	class Widget
	{
	private:
		friend class UIManager;
	public:
		Widget(Game *game);
		virtual ~Widget();

		virtual void Update(float dt) = 0;
		virtual void UpdateInGame(float dt) = 0;
		virtual void Resize(unsigned int width, unsigned int height);

		void SetEnabled(bool enable);
		bool IsEnabled() const { return isEnabled; }

		void SetMaterial(MaterialInstance *matInstance);
		void SetRect(const Rect &rect);
		void SetRect(const glm::vec2 &pos, const glm::vec2 &size);
		void SetRectPosPercent(const glm::vec2 &posPercent);
		virtual void SetRectSize(const glm::vec2 &size);
		void SetColorTintRGB(const glm::vec3 &color) { params.colorTint = glm::vec4(color.x, color.y, color.z, params.colorTint.w); }
		void SetColorTintRGBA(const glm::vec4 &color) { params.colorTint = color; }
		void SetAlpha(float alpha) { params.colorTint.w = alpha; }
		void SetDepth(float z) { params.depth = z; }

		const Rect &GetRect() const { return rect; }
		const glm::vec2 &GetPosPercent() const { return positionPercent; }
		WidgetType GetType() const { return type; }
		const glm::vec4 &GetColorTint() const { return params.colorTint; }
		const float GetDepth() const { return params.depth; }
		const WidgetShaderParams &GetParams() const { return params; }
		const glm::mat4 &GetTransform();

		virtual void Serialize(Serializer &s);
		virtual void Deserialize(Serializer &s);

		// Script functions
		glm::vec2 GetRectSize() const { return rect.size; }

		static Button *CastToButton(Widget *w);

	protected:
		Game *game;
		Rect rect;
		glm::vec2 positionPercent;
		WidgetType type;
		MaterialInstance *matInstance;
		bool isEnabled;
		WidgetShaderParams params;
		glm::mat4 transform;
	};
}
