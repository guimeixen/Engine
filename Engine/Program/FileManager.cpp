#include "FileManager.h"

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

		isInit = true;
	}

	

	std::ifstream FileManager::OpenForReading(const std::string &path, std::ios_base::openmode mode)
	{
		//FILE *f = fopen(path.c_str(), mode);
		std::ifstream f(path.c_str(), mode);
		return f;
	}

	std::ofstream FileManager::OpenForWriting(const std::string &path, std::ios_base::openmode mode)
	{
		//FILE *f = fopen(path.c_str(), mode);
		std::ofstream f(path.c_str(), mode);
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