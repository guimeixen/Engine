#pragma once

#include <string>
#include <fstream>

namespace Engine
{
	class FileManager
	{
	public:
		FileManager();

		void Init();

		std::ifstream OpenForReading(const std::string &path, std::ios_base::openmode mode = std::ios_base::in);
		std::ofstream OpenForWriting(const std::string &path, std::ios_base::openmode mode = std::ios_base::out);
		// File must have been opened with std::ios::ate
		char *ReadEntireFile(std::ifstream &file, bool isBinary);

	private:
		bool isInit;
	};
}
