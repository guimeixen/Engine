#include "Font.h"

#include "Program\Log.h"
#include "ResourcesLoader.h"
#include "Shader.h"
#include "Buffers.h"
#include "Renderer.h"
#include "VertexArray.h"
#include "Material.h"

#include "include\glm\gtc\matrix_transform.hpp"

#include <fstream>
#include <iostream>

namespace Engine
{
	Font::Font()
	{
		maxCharsBuffer = MAX_QUADS / 6;
		curQuadCount = 0;
		vertexBuffer = nullptr;
		enabled = true;
		mesh = {};
	}

	Font::~Font()
	{
	}

	void Font::Init(Renderer *renderer, ScriptManager &scriptManager, const std::string &fontPath, const std::string &fontAtlasPath)
	{
		this->renderer = renderer;

		mesh = {};

		vertexBuffer = renderer->CreateVertexBuffer(nullptr, maxCharsBuffer * 6 * sizeof(VertexPOS2D_UV_COLOR), BufferUsage::DYNAMIC);

		VertexAttribute attribs[2] = {};
		attribs[0].count = 4;
		attribs[0].offset = 0;

		attribs[1].count = 4;
		attribs[1].offset = 4 * sizeof(float);

		VertexInputDesc desc = {};
		desc.stride = 8 * sizeof(float);
		desc.attribs = { attribs[0], attribs[1] };

		mesh.vao = renderer->CreateVertexArray(desc, vertexBuffer, nullptr);

		matInstance = renderer->CreateMaterialInstanceFromBaseMat(scriptManager, "Data/Resources/Materials/text_mat.lua", { desc });
		TextureParams params = { TextureWrap::REPEAT, TextureFilter::LINEAR, TextureFormat::RGBA, TextureInternalFormat::RGBA8, TextureDataType::UNSIGNED_BYTE, false, false };
		textAtlas = renderer->CreateTexture2D(fontAtlasPath, params);
		matInstance->textures[0] = textAtlas;
		renderer->UpdateMaterialInstance(matInstance);

		ReadFontFile(fontPath);
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

	void Font::ReadFontFile(const std::string &fontPath)
	{
		std::ifstream file(fontPath);

		if (!file.is_open())
		{
			Log::Print(LogLevel::LEVEL_ERROR, "Error! Failed to load font file");
			std::cout << fontPath << "\n";
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

		file.close();
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

				quadsBuffer[curQuadCount * 6 + 0] = { glm::vec4(topLeft.x,		topLeft.y,		val1, val4), t.color };
				quadsBuffer[curQuadCount * 6 + 1] = { glm::vec4(bottomLeft.x,	bottomLeft.y,	val1, val3), t.color };
				quadsBuffer[curQuadCount * 6 + 2] = { glm::vec4(bottomRight.x,	bottomRight.y,	val2, val3), t.color };
				quadsBuffer[curQuadCount * 6 + 3] = { glm::vec4(topLeft.x,		topLeft.y,		val1, val4), t.color };
				quadsBuffer[curQuadCount * 6 + 4] = { glm::vec4(bottomRight.x,	bottomRight.y,	val2, val3), t.color };
				quadsBuffer[curQuadCount * 6 + 5] = { glm::vec4(topRight.x,		topRight.y,		val2, val4), t.color };

				x += (c.advance - paddingWidth) * t.scale.x;

				curQuadCount++;
			}
		}

		if (curQuadCount > 0)
		{
			vertexBuffer->Update(quadsBuffer, curQuadCount * 6 * sizeof(VertexPOS2D_UV_COLOR), 0);
			mesh.vertexCount = curQuadCount * 6;
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
	}

	void Font::Resize(unsigned int width, unsigned int height)
	{
		this->width = width;
		this->height = height;
	}
}
