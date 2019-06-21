#include "Font.h"

#include "Program/Log.h"
#include "Program/FileManager.h"
#include "ResourcesLoader.h"
#include "Shader.h"
#include "Buffers.h"
#include "Renderer.h"
#include "VertexArray.h"
#include "Material.h"

#include "include/glm/gtc/matrix_transform.hpp"

namespace Engine
{
	Font::Font()
	{
		maxCharsBuffer = MAX_QUADS;
		curQuadCount = 0;
		vertexBuffer = nullptr;
		enabled = true;
		mesh = {};
	}

	bool Font::Init(Renderer *renderer, ScriptManager &scriptManager, const std::string &fontPath, const std::string &fontAtlasPath)
	{
		this->renderer = renderer;
		this->fontPath = fontPath;

		Log::Print(LogLevel::LEVEL_INFO, "Init font\n");

		mesh = {};

		Log::Print(LogLevel::LEVEL_INFO, "Creating vertex buffer\n");
		vertexBuffer = renderer->CreateVertexBuffer(nullptr, MAX_QUADS * 4 * sizeof(VertexPOS2D_UV_COLOR), BufferUsage::DYNAMIC);

		// Given that the Vita doesn't support non-indexed drawing, we need to use an index buffer
		// So we just need to create one with the space for the maximum number of characters and fill with the generated indices and never touch it again

		unsigned short indices[MAX_QUADS * 6];

		unsigned short index = 0;
		for (unsigned short i = 0; i < MAX_QUADS * 6; i += 6)
		{
			indices[i]	   = index;
			indices[i + 1] = index + 1;
			indices[i + 2] = index + 2;
			indices[i + 3] = index + 0;
			indices[i + 4] = index + 2;
			indices[i + 5] = index + 3;
			index += 4;
		}

		//unsigned short indices[] = { 0,1,2, 0,2,3 };

		Log::Print(LogLevel::LEVEL_INFO, "Creating index buffer\n");
		Buffer *indexBuffer = renderer->CreateIndexBuffer(indices, sizeof(indices), BufferUsage::STATIC);

		VertexAttribute attribs[2] = {};
		attribs[0].count = 4;
		attribs[0].offset = 0;

		attribs[1].count = 4;
		attribs[1].offset = 4 * sizeof(float);

		VertexInputDesc desc = {};
		desc.stride = 8 * sizeof(float);
		desc.attribs = { attribs[0], attribs[1] };

		mesh.vao = renderer->CreateVertexArray(desc, vertexBuffer, indexBuffer);

		Log::Print(LogLevel::LEVEL_INFO, "Creating text material instance\n");
		matInstance = renderer->CreateMaterialInstanceFromBaseMat(scriptManager, "Data/Materials/text_mat.lua", { desc });

		Log::Print(LogLevel::LEVEL_INFO, "Creating font texture\n");
		TextureParams params = { TextureWrap::REPEAT, TextureFilter::LINEAR, TextureFormat::RGBA, TextureInternalFormat::RGBA8, TextureDataType::UNSIGNED_BYTE, false, false };
		textAtlas = renderer->CreateTexture2D(fontAtlasPath, params);

		matInstance->textures[0] = textAtlas;
		renderer->UpdateMaterialInstance(matInstance);

		if (!ReadFontFile(fontPath))
			return false;

		return true;
	}

	void Font::Reload(const std::string &fontPath)
	{
		renderer->RemoveTexture(textAtlas);
		std::string atlasPath = fontPath;
		atlasPath.pop_back();
		atlasPath.pop_back();
		atlasPath.pop_back();
		atlasPath += "png";

		TextureParams params = { TextureWrap::REPEAT, TextureFilter::LINEAR, TextureFormat::RGBA, TextureInternalFormat::RGBA8, TextureDataType::UNSIGNED_BYTE, false, false };
		textAtlas = renderer->CreateTexture2D(atlasPath, params);
		matInstance->textures[0] = textAtlas;
		renderer->UpdateMaterialInstance(matInstance);

		ReadFontFile(fontPath);
	}

	void Font::AddText(const std::string &text, const glm::vec2 &pos, const glm::vec2 &scale, const glm::vec4 &color)
	{
		if (enabled)
			textBuffer.push_back({ text, pos, scale, color });
	}

	void Font::AddText(const std::string &text, const Rect &rect)
	{
		if (enabled)
			textBuffer.push_back({ text, rect.position, rect.size, glm::vec4(1.0f) });
	}

	glm::vec2 Font::CalculateTextSize(const std::string &text, const glm::vec2 &scale)
	{
		glm::vec2 size = glm::vec2();

		for (const char &cc : text)
		{
			const Character &c = characters[cc];
			size.x += (c.advance - paddingWidth) * scale.x;
			float h = c.size.y * scale.y;
			size.y = std::max(size.y, h);
		}

		return size;
	}

	glm::vec2 Font::CalculateCharSize(char c, const glm::vec2 &scale)
	{
		glm::vec2 size = glm::vec2();

		const Character &cc = characters[c];
		size.x += (cc.advance - paddingWidth) * scale.x;
		size.y = cc.size.y * scale.y;

		return size;
	}

	bool Font::ReadFontFile(const std::string &fontPath)
	{
		std::ifstream file = renderer->GetFileManager()->OpenForReading(fontPath);

		if (!file.is_open())
		{
			Log::Print(LogLevel::LEVEL_ERROR, "ERROR -> Failed to load font file: %s\n", fontPath);
			return false;
		}

		std::string line;
		std::string temp;

		while (std::getline(file, line))
		{
			if (line.substr(0, 10) == "info face=")
			{
				size_t pos = line.find("padding=");

				if (pos != std::string::npos && line.substr(pos, 8) == "padding=")
				{
					padding[0] = std::stoi(line.substr(pos + 8));

					size_t next = line.find(",", pos);
					padding[1] = std::stoi(line.substr(next + 1));

					next = line.find(",", next + 1);			// Add plus one so we go forward, otherwise we would keep getting the position of the first comma
					padding[2] = std::stoi(line.substr(next + 1));

					next = line.find(",", next + 1);
					padding[3] = std::stoi(line.substr(next + 1));

					paddingWidth = padding[1] + padding[3];
				}
			}

			// Only add a character if this line describes the character 
			if (line.substr(0, 8) == "char id=")
			{
				Character c = {};
				c.id = std::stoi(line.substr(8));

				size_t pos = line.find("x=");

				if (pos != std::string::npos && line.substr(pos, 2) == "x=")
				{
					c.uv.x = std::stof(line.substr(pos + 2));
				}

				pos = line.find("y=");

				if (pos != std::string::npos && line.substr(pos, 2) == "y=")
				{
					c.uv.y = std::stof(line.substr(pos + 2));
				}

				pos = line.find("width=");

				if (pos != std::string::npos && line.substr(pos, 6) == "width=")
				{
					c.size.x = std::stof(line.substr(pos + 6));
				}

				pos = line.find("height=");

				if (pos != std::string::npos && line.substr(pos, 7) == "height=")
				{
					c.size.y = std::stof(line.substr(pos + 7));
				}

				pos = line.find("xoffset=");

				if (pos != std::string::npos && line.substr(pos, 8) == "xoffset=")
				{
					c.offset.x = std::stof(line.substr(pos + 8));
				}

				pos = line.find("yoffset=");

				if (pos != std::string::npos && line.substr(pos, 8) == "yoffset=")
				{
					c.offset.y = std::stof(line.substr(pos + 8));
				}

				pos = line.find("xadvance=");

				if (pos != std::string::npos && line.substr(pos, 9) == "xadvance=")
				{
					c.advance = std::stof(line.substr(pos + 9));
				}

				characters[c.id] = c;
			}
		}

		return true;
	}

	void Font::PrepareText()
	{
		for (size_t i = 0; i < textBuffer.size(); i++)
		{
			const Text &t = textBuffer[i];

			float x = t.pos.x;

			// We have a limit on how much text we can render
			if (curQuadCount > maxCharsBuffer)
				break;

			for (const char &cc : t.text)
			{
				// This second if breaks this inner loop the other if breaks the first one
				if (curQuadCount > maxCharsBuffer)
					break;

				const Character &c = characters[cc];

				float xpos = x + c.offset.x * t.scale.x;
				float ypos = t.pos.y - (c.size.y + c.offset.y) * t.scale.y;
				float w = c.size.x * t.scale.x;
				float h = c.size.y * t.scale.y;

				float val1 = c.uv.x / textAtlas->GetWidth();
				float val2 = (c.uv.x + c.size.x) / textAtlas->GetWidth();
				float val4 = c.uv.y / textAtlas->GetHeight();
				float val3 = (c.uv.y + c.size.y) / textAtlas->GetHeight();

				glm::vec4 topLeft = glm::vec4(xpos, ypos + h, 0.0f, 1.0f);
				glm::vec4 bottomLeft = glm::vec4(xpos, ypos, 0.0f, 1.0f);
				glm::vec4 bottomRight = glm::vec4(xpos + w, ypos, 0.0f, 1.0f);
				glm::vec4 topRight = glm::vec4(xpos + w, ypos + h, 0.0f, 1.0f);

				quadsBuffer[curQuadCount * 4 + 0] = { glm::vec4(topLeft.x,		topLeft.y,		val1, val4), t.color };
				quadsBuffer[curQuadCount * 4 + 1] = { glm::vec4(bottomLeft.x,	bottomLeft.y,	val1, val3), t.color };
				quadsBuffer[curQuadCount * 4 + 2] = { glm::vec4(bottomRight.x,	bottomRight.y,	val2, val3), t.color };
				quadsBuffer[curQuadCount * 4 + 3] = { glm::vec4(topRight.x,		topRight.y,		val2, val4), t.color };

				x += (c.advance - paddingWidth) * t.scale.x;

				curQuadCount++;
			}
		}

		if (curQuadCount > 0)
		{
			vertexBuffer->Update(quadsBuffer, curQuadCount * 4 * sizeof(VertexPOS2D_UV_COLOR), 0);
			//mesh.vertexCount = curQuadCount * 4;
			mesh.indexCount = curQuadCount * 6;
		}
	}

	void Font::EndTextUpdate()
	{
		textBuffer.clear();
		curQuadCount = 0;
	}

	void Font::Dispose()
	{
		if (textAtlas)
			textAtlas->RemoveReference();

		if (mesh.vao)
		{
			delete mesh.vao;
			mesh.vao = nullptr;
		}

		Log::Print(LogLevel::LEVEL_INFO, "Disposing font\n");
	}

	void Font::Resize(unsigned int width, unsigned int height)
	{
		this->width = width;
		this->height = height;
	}
}
