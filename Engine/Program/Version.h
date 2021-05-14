#pragma once

namespace Engine
{
	static const unsigned int MAJOR_VERSION = 0;
	// Some additions, could have breaking changes
	static const unsigned int MINOR_VERSION = 0;
	// Bug fixes, minor additions, etc. Compatibility with older versions is kept
	static const unsigned int PATCH_VERSION = 5;

	const char* GetVersionString();
}
