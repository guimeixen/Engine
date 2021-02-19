#include <string>
#include "Version.h"

namespace Engine
{
	static const std::string versionStr = std::to_string(MAJOR_VERSION) + '.' + std::to_string(MINOR_VERSION) + '.' + std::to_string(PATCH_VERSION);

	const char* GetVersionString()
	{
		return versionStr.c_str();
	}
}