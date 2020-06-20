#include "FileManager.h"

#ifdef VITA
#include "Program/Log.h"
#include "psp2/appmgr.h"
#include "psp2/kernel/processmgr.h"
#endif

namespace Engine
{
	FileManager::FileManager()
	{
		isInit = false;
	}

	void FileManager::Init()
	{
		if (isInit)
			return;

#ifdef VITA
		/*SceUID id = sceKernelGetProcessId();
		char titleID[10] = {};
		sceAppMgrAppParamGetString(id, 12, titleID, 10);
		Engine::Log::Print(Engine::LogLevel::LEVEL_INFO, "%s\n", titleID);

		vitaAppPath = "ux0:app/";
		vitaAppPath += titleID;
		vitaAppPath += '/';*/

		vitaAppPath = "app0:";
#endif

		isInit = true;
	}

	

	std::ifstream FileManager::OpenForReading(const std::string &path, std::ios_base::openmode mode)
	{
#ifdef VITA
		std::string completePath = vitaAppPath + path;

		Log::Print(LogLevel::LEVEL_INFO, "Opening file: %s\n", completePath.c_str());

		//FILE *f = fopen(completePath.c_str(), mode);
		std::ifstream f(completePath.c_str(), mode);
#else
		//FILE *f = fopen(path.c_str(), mode);
		std::ifstream f(path.c_str(), mode);
#endif
		return f;
	}

	std::ofstream FileManager::OpenForWriting(const std::string &path, std::ios_base::openmode mode)
	{
#ifdef VITA
		std::string completePath = vitaAppPath + path;

		Log::Print(LogLevel::LEVEL_INFO, "Opening file: %s\n", completePath.c_str());

		//FILE *f = fopen(completePath.c_str(), mode);
		std::ofstream f(completePath.c_str(), mode);
#else
		//FILE *f = fopen(path.c_str(), mode);
		std::ofstream f(path.c_str(), mode);
#endif
		return f;
	}

	char *FileManager::ReadEntireFile(std::ifstream &file, bool isBinary)
	{
		if (!file.is_open())
			return nullptr;

		size_t size = (size_t)file.tellg();
		char *buf = nullptr;

		if (isBinary)
			buf = new char[size];
		else
		{
			buf = new char[size + 1];
			memset(buf, 0, size + 1);
		}

		file.seekg(0);				// Go back the beginning of the file
		file.read(buf, size);		// Now read it all at once

		if (!isBinary)
			buf[size] = 0;

		/*fseek(f, 0, SEEK_END);
		long size = ftell(f);
		fseek(f, 0, SEEK_SET);

		char *buf = (char*)malloc(size + 1);
		memset(buf, 0, size + 1);
		fread(buf, 1, size, f);

		buf[size] = 0;*/

		return buf;
	}
}