#pragma once

#include "VertexTypes.h"
#include "Mesh.h"

#include "Game\UI\Widget.h"
#include "Game\ComponentManagers\ScriptManager.h"

#include <string>
#include <vector>
#include <map>

#define MAX_QUADS 2048 * 6	// * 6 because a quad has 6 vertices

namespace Engine
{
	class Texture;
	class Shader;
	class DataManager;
	class Buffer;
	class Renderer;
	struct MaterialInstance;

	struct Character
	{
		int id;
		glm::vec2 uv;
		glm::vec2 size;
		glm::vec2 offset;
		float advance;
	};

	struct Text
	{
		std::string text;
		glm::vec2 pos;
		glm::vec2 scale;
		glm::vec4 color;
	};

	class Font
	{
	public:
		Font();
		~Font();

		void Init(Renderer *renderer, ScriptManager &scriptManager, const std::string &fontPath, const std::string &fontAtlasPath);
		void Reload(const std::string &fontPath);
		void PrepareText();
		void EndTextUpdate();
		void Dispose();
		void Enable(bool enable) { enabled = enable; }

		void Resize(unsigned int width, unsigned int height);

		void AddText(const std::string &text, const glm::vec2 &pos, const glm::vec2 &scale, const glm::vec4 &color = glm::vec4(1.0f));
		void AddText(const std::string &text, const Rect &rect);
		glm::vec2 CalculateTextSize(const std::string &text, const glm::vec2 &scale);
		glm::vec2 CalculateCharSize(char c, const glm::vec2 &scale);

		Buffer *GetVertexBuffer() const { return vertexBuffer; }
		Texture *GetFontAtals() const { return textAtlas; }
		unsigned int GetCurrentCharCount() const { return curQuadCount; }
		const Mesh &GetMesh() const { return mesh; }
		MaterialInstance *GetMaterialInstance() const { return matInstance; }

	private:
		void ReadFontFile(const std::string &fontPath);

	private:
		Renderer *renderer;
		Texture *textAtlas;
		unsigned int width;
		unsigned int height;
		Mesh mesh;
		MaterialInstance *matInstance;
		Buffer *vertexBuffer;

		bool enabled;

		std::vector<Text> textBuffer;
		std::map<int, Character> characters;
		VertexPOS2D_UV_COLOR quadsBuffer[MAX_QUADS];
		unsigned int maxCharsBuffer;					// max number of characters that are stored. Once it reaches the max we render them and start storing them again
		unsigned int curQuadCount;
		int paddingWidth;
		int padding[4];
	};
}
