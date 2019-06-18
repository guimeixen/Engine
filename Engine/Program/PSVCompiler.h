#pragma once

#include <string>

namespace Engine
{
	class Game;

	class PSVCompiler
	{
	public:
		void Compile(Game *game, const std::string &curProjectDir, const std::string &curProjectName, const std::string &appName, const std::string &appTitleID);

	private:

	};
}
