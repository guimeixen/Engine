#include "PSVCompiler.h"

#include "Utils.h"

#include <fstream>

namespace Engine
{
	void PSVCompiler::Compile(const std::string &curProjectDir, const std::string &appName, const std::string &appTitleID)
	{
		std::string folderPath = curProjectDir + "/PSVita_Build";

		if (!utils::DirectoryExists(folderPath))
			utils::CreateFolder(folderPath.c_str());

		std::string cmakelists = "cmake_minimum_required(VERSION 2.8)\n";
		cmakelists += "cmake_policy(SET CMP0015 NEW)\n";
		cmakelists += "if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)\n\tif (DEFINED ENV{VITASDK})\n\t\tset(CMAKE_TOOLCHAIN_FILE \"$ENV{VITASDK}/share/vita.toolchain.cmake\" CACHE PATH \"toolchain file\")\n\telse()\n\t\tmessage(FATAL_ERROR \"Please define VITASDK to point to your SDK path!\")\n\tendif()\nendif()\n";
		cmakelists += "project(vitaTestApp)\n";
		cmakelists += "include(\"${VITASDK}/share/vita.cmake\" REQUIRED)\n";
		cmakelists += "set(VITA_APP_NAME \"";
		cmakelists += appName;
		cmakelists += "\")\n";
		cmakelists += "set(VITA_TITLEID \"";
		cmakelists += appTitleID;
		cmakelists += "\")\n";
		cmakelists += "set(VITA_VERSION \"01.00\")\n";			// Make user give version
		cmakelists += "set(CMAKE_C_FLAGS \"${CMAKE_C_FLAGS} -Wall\")\nset(CMAKE_CXX_FLAGS \"${CMAKE_CXX_FLAGS} -std=c++11\")\nset(VITA_MKSFOEX_FLAGS \"${VITA_MKSFOEX_FLAGS} -d PARENTAL_LEVEL=1\")\n";
		cmakelists += "include_directories(\n\t../../../../\n\t../../../../Engine\n\t../../../../include/bullet\n)\n";
		cmakelists += "link_directories(\n\t${CMAKE_CURRENT_BINARY_DIR}\n\t../../../../\n)\n";
		cmakelists += "add_executable(vitaTestApp main.cpp)\n";
		cmakelists += "target_link_libraries(vitaTestApp\n\tVitaEngine\n\tLua5_3_4\n\tassimp\n\tzlib\n\tBulletDynamics\n\tBulletCollision\n\tBulletSoftBody\n\tLinearMath\n\tSceLibKernel_stub\n\tSceGxm_stub\n\tSceDisplay_stub\n\tSceCtrl_stub\n)\n";
		cmakelists += "vita_create_self(vitaTestApp.self vitaTestApp UNSAFE)\n";
		cmakelists += "vita_create_vpk(vitaTestApp.vpk ${VITA_TITLEID} vitaTestApp.self VERSION ${VITA_VERSION} NAME ${VITA_APP_NAME} FILE sce_sys/icon0.png sce_sys/icon0.png\nFILE sce_sys/livearea/contents/bg.png sce_sys/livearea/contents/bg.png\nFILE sce_sys/livearea/contents/startup.png sce_sys/livearea/contents/startup.png\nFILE sce_sys/livearea/contents/template.xml sce_sys/livearea/contents/template.xml)\n";

		std::ofstream file = std::ofstream(folderPath + "/CMakeLists.txt");
		if (file.is_open())
		{
			file << cmakelists;
		}

		std::ofstream main = std::ofstream(folderPath + "/main.cpp");
		if (main.is_open())
		{
			main << "#include \"PSVitaApplication.h\"\n\
			class MyApplication : public Engine::PSVitaApplication\
			{public:\
			bool Init(){\
			if (!PSVitaApplication::Init())\
			return false;\
			return true;}\
			void Update(){PSVitaApplication::Update();}\
			void Render(){PSVitaApplication::Render();}};\
			int main(){\
			MyApplication app;\
			if (!app.Init())\
			return 1;\
			return app.Run();}";
		}
	}
}
