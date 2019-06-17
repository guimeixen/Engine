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

		std::ifstream Open(const std::string &path, std::ios_base::openmode mode = std::ios_base::in);
		//void Close(FILE *f);
		// File must have been opened with std::ios::ate
		char *ReadEntireFile(std::ifstream &file, bool isBinary);

#ifdef VITA
		const std::string &GetAppPath() const { return vitaAppPath; }
#endif

	private:
		bool isInit;
#ifdef VITA
		std::string vitaAppPath;
#endif
	};
}
