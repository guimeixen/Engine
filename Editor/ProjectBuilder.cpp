#include "ProjectBuilder.h"

#include "Program/Utils.h"
#include "Program/Log.h"

#include <iostream>
#include <filesystem>

void ProjectBuilder::Export(const std::string& curProjectDir, const std::string& curProjectName)
{
	std::string folderPath = curProjectDir + "/" + curProjectName + "_Build";

	if (!Engine::utils::DirectoryExists(folderPath))
		Engine::utils::CreateDir(folderPath.c_str());

	// Get MSBUILD path
	// It's more simple to just save the ouput to a file and read it, than to uses popen or CreateProcess on windows
	std::string cmd = "vswhere.exe -find MSBUILD>path.txt";
	if (std::system(cmd.c_str()) != 0)
	{
		Engine::Log::Print(Engine::LogLevel::LEVEL_ERROR, "Getting MSBUILD path failed!\n");
		return;
	}

	std::ifstream file("path.txt");

	if (!file.is_open())
	{
		Engine::Log::Print(Engine::LogLevel::LEVEL_ERROR, "Error -> Failed to open file : path.txt\n");
		return;
	}

	std::string msbuildPath;
	std::getline(file, msbuildPath);
	file.close();

	msbuildPath += "\\Current\\Bin\\MSBuild.exe";

	std::filesystem::remove("path.txt");

	std::ofstream mainFile = std::ofstream(folderPath + "/main.cpp");
	if (mainFile.is_open())
	{
		mainFile << "#include \"Application.h\"\n\
			class MyApplication : public Engine::Application\n\
			{public:\n\
			bool Init(Engine::GraphicsAPI api, unsigned int width, unsigned int height){\n\
			if (!Engine::Application::Init(api, width, height))\n\
			return false;\n\
			if (!game.LoadProject(\"" + curProjectName + "\"))\n\
			return false;\n\
			return true;}\n\
			void Update(){Application::Update();}\n\
			void Render(){Application::Render();}};\n\
			int main(){\n\
			const unsigned int WIDTH = 1280;\n\
			const unsigned int HEIGHT = 720;\n\
			MyApplication app;\
			if (!app.Init(Engine::GraphicsAPI::Vulkan, WIDTH, HEIGHT))\n\
			return 1;\n\
			return app.Run();}\n";
	}
	mainFile.close();


#ifdef _WIN32
	std::ofstream buildFile(folderPath + "/build.bat");
	if (buildFile.is_open())
	{
		buildFile << "xcopy /k/c/y \"..\\..\\..\\Build\\Build.vcxproj\" \".\\\"\n";				// Copy the project file, making sure the previous dir is specified as ..\ instead of ../ otherwise xcopy won't copy
		buildFile << "\"" + msbuildPath + "\" Build.vcxproj\n";
	}
	buildFile.close();
#endif
/*#elif __linux__
	std::ofstream buildFile(folderPath + "/build.sh");
	if (buildFile.is_open())
	{
	}
	buildFile.close();
#endif*/

	cmd = "cd " + folderPath + " & build.bat";
	if (std::system(cmd.c_str()) != 0)
	{
		Engine::Log::Print(Engine::LogLevel::LEVEL_ERROR, "Build failed!\n");
	}
}
