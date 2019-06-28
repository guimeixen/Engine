#pragma once

#include "Game\EntityManager.h"
#include "Graphics\RendererStructs.h"
#include "Graphics\Mesh.h"

#include "Game\Script.h"

#include "Widget.h"

#include <vector>
#include <unordered_map>

namespace Engine
{
	class Game;
	class Renderer;
	class Texture;
	class Image;
	struct MaterialInstance;

	struct WidgetInstance
	{
		Entity e;
		Widget *w;
	};

	class UIManager : public RenderQueueGenerator
	{
	public:
		void Init(Game *game);
		void PartialDispose();
		void Dispose();
		void Update(float dt);
		void UpdateInGame(float dt);
		void Resize(unsigned int width, unsigned int height);

		void Cull(unsigned int passAndFrustumCount, unsigned int *passIds, const Frustum *frustums, std::vector<VisibilityIndices*> &out) override;
		void GetRenderItems(unsigned int passCount, unsigned int *passIds, const VisibilityIndices &visibility, RenderQueue &outQueues) override;

		Widget *AddButton(Entity e);
		Widget *AddText(Entity e);
		Widget *AddImage(Entity e);
		Widget *AddEditText(Entity e);

		void DuplicateWidget(Entity e, Entity newE);
		void SetWidgetEnabled(Entity e, bool enable);
		void LoadWidgetFromPrefab(Serializer &s, Entity e);
		void RemoveWidget(Entity e);
		bool HasWidget(Entity e) const;
		Widget *GetWidget(Entity e) const;

		Entity PerformRaycast(const glm::vec2 &point);

		void ShowCursor(bool show) { showCursor = show; }
		bool IsCursorVisible() const { return showCursor; }
		const glm::vec2 &GetCursorPos() const { return cursorPos; }
		void SetCursorPos(const glm::vec2 &pos) { cursorPos = pos; }

		void Serialize(Serializer &s) const;
		void Deserialize(Serializer &s, bool reload = false);

	private:
		void InsertWidgetInstance(const WidgetInstance &wi);

	private:
		Game *game;
		Mesh mesh;
		std::vector<WidgetInstance> widgets;
		std::unordered_map<unsigned int, unsigned int> map;
		unsigned int usedWidgets;
		unsigned int disabledWidgets;

		Image *cursor;
		bool showCursor;
		glm::vec2 cursorPos;
	};
}
