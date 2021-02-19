#pragma once

namespace Engine
{
	static const unsigned int MAJOR_VERSION = 0;
	// Minor additions, could have breaking changes
	static const unsigned int MINOR_VERSION = 0;
	// Bug fixes, etc. Compatibility with older versions is kept
	static const unsigned int PATCH_VERSION = 1;

	const char* GetVersionString();
}
