#include "GXMTexture2D.h"

#include "Program/Log.h"
#include "Program/FileManager.h"
#include "GXMUtils.h"

#include "include/stb_image.h"

namespace Engine
{
	GXMTexture2D::GXMTexture2D()
	{
		
	}

	GXMTexture2D::~GXMTexture2D()
	{
		gxmutils::graphicsFree(uid);
		gxmutils::graphicsFree(paletteUID);
	}

	bool GXMTexture2D::Load(FileManager * fileManager, const std::string & path, const TextureParams & params, bool storeTextureData)
	{
		this->path = fileManager->GetAppPath();
		this->path += path;

		Log::Print(LogLevel::LEVEL_INFO, "Loading texture: %s\n", this->path.c_str());

		AddReference();
		data = nullptr;
		this->params = params;
		type = TextureType::TEXTURE2D;
		this->storeTextureData = storeTextureData;

		unsigned char *image = nullptr;

		int texWidth = 0;
		int textHeight = 0;
		int channelsInFile = 0;
		int channelsInData = 0;
		unsigned int size = 0;

		//stbi_set_flip_vertically_on_load(1);

		if (params.format == TextureFormat::RGB)
		{
			/*image = stbi_load(this->path.c_str(), &texWidth, &textHeight, &channelsInFile, STBI_rgb);
			channelsInData = 3;
			size = texWidth * height * 3 * sizeof(unsigned char);*/
			Log::Print(LogLevel::LEVEL_ERROR, "ERROR -> 3 channel textures not supported for now %s\n", path);
			return false;
		}
		else if (params.format == TextureFormat::RGBA)
		{
			image = stbi_load(this->path.c_str(), &texWidth, &textHeight, &channelsInFile, STBI_rgb_alpha);
			//Log::Print(LogLevel::LEVEL_INFO, "%s\n", stbi_failure_reason());
			channelsInData = 4;
			size = (unsigned int)texWidth * (unsigned int)textHeight * 4 * sizeof(unsigned char);		
		}
		else if (params.format == TextureFormat::RED)
		{
			image = stbi_load(this->path.c_str(), &texWidth, &textHeight, &channelsInFile, STBI_grey);
			channelsInData = 1;
			size = (unsigned int)texWidth * (unsigned int)textHeight * 1 * sizeof(unsigned char);
		}

		if (!image)
		{
			Log::Print(LogLevel::LEVEL_ERROR, "ERROR -> Failed to load texture: %s\n", this->path.c_str());
			return false;
		}

		width = (unsigned int)texWidth;
		height = (unsigned int)textHeight;

		Log::Print(LogLevel::LEVEL_INFO, "width: %u height: %u size: %u\n", width, height, size);

		void *texData = gxmutils::graphicsAlloc(SCE_KERNEL_MEMBLOCK_TYPE_USER_CDRAM_RW, SCE_GXM_MEMORY_ATTRIB_RW, size, &uid);

		if (!texData)
		{
			Log::Print(LogLevel::LEVEL_ERROR, "ERROR -> Failed to allocate data for texture  %s\n", path.c_str());
			stbi_image_free(image);
			return false;
		}

		//memset(data, 0, size);
		memcpy(texData, image, size);

		sceGxmTextureInitLinear(&gxmTexture, texData, SCE_GXM_TEXTURE_FORMAT_A8B8G8R8, width, height, 0);

		Log::Print(LogLevel::LEVEL_INFO, "Init texture\n");

		/*if ((SCE_GXM_TEXTURE_FORMAT_A8B8G8R8 & 0x9f000000U) == SCE_GXM_TEXTURE_BASE_FORMAT_P8)
		{
			Log::Print(LogLevel::LEVEL_INFO, "Allocating texture pallete\n");
			const int palleteSize = 256 * sizeof(unsigned int);

			void *texturePalette = gxmutils::graphicsAlloc(SCE_KERNEL_MEMBLOCK_TYPE_USER_CDRAM_RW, SCE_GXM_MEMORY_ATTRIB_READ, palleteSize, &paletteUID);

			if (!texturePalette)
			{
				Log::Print(LogLevel::LEVEL_ERROR, "ERROR -> Failed to allocate palette for texture  %s\n", path);
				stbi_image_free(image);
				return false;
			}

			memset(texturePalette, 0, palleteSize);

			sceGxmTextureSetPalette(gxmTexture, texturePalette);
		}*/

		sceGxmTextureSetMinFilter(&gxmTexture, SCE_GXM_TEXTURE_FILTER_LINEAR);
		sceGxmTextureSetMagFilter(&gxmTexture, SCE_GXM_TEXTURE_FILTER_LINEAR);
		sceGxmTextureSetUAddrMode(&gxmTexture, SCE_GXM_TEXTURE_ADDR_REPEAT);
		sceGxmTextureSetVAddrMode(&gxmTexture, SCE_GXM_TEXTURE_ADDR_REPEAT);

		stbi_image_free(image);

		return true;
	}

	void GXMTexture2D::Bind(unsigned int slot) const
	{
	}

	void GXMTexture2D::BindAsImage(unsigned int slot, unsigned int mipLevel, bool layered, ImageAccess access, TextureInternalFormat format) const
	{
	}

	void GXMTexture2D::Unbind(unsigned int slot) const
	{
	}

	void GXMTexture2D::Clear()
	{
	}
}
