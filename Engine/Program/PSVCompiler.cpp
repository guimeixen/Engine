#include "PSVCompiler.h"

#include "Game/Game.h"
#include "Utils.h"
#include "Program/Log.h"
#include "Graphics/ResourcesLoader.h"
#include "Graphics/Shader.h"

#include <fstream>

namespace Engine
{
	void PSVCompiler::Compile(Game *game, const std::string &curProjectDir, const std::string &curProjectName, const std::string &appName, const std::string &appTitleID)
	{
		std::string folderPath = curProjectDir + "/PSVita_Build";

		if (!utils::DirectoryExists(folderPath))
			utils::CreateDir(folderPath.c_str());

		std::string appNameNoSpaces = appName;
		appNameNoSpaces.erase(remove_if(appNameNoSpaces.begin(), appNameNoSpaces.end(), isspace), appNameNoSpaces.end());

		std::string cmakelists = "cmake_minimum_required(VERSION 2.8)\n";
		cmakelists += "cmake_policy(SET CMP0015 NEW)\n";
		cmakelists += "if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)\n\tif (DEFINED ENV{VITASDK})\n\t\tset(CMAKE_TOOLCHAIN_FILE \"$ENV{VITASDK}/share/vita.toolchain.cmake\" CACHE PATH \"toolchain file\")\n\telse()\n\t\tmessage(FATAL_ERROR \"Please define VITASDK to point to your SDK path!\")\n\tendif()\nendif()\n";
		cmakelists += "project(" + appNameNoSpaces + ")\n";
		cmakelists += "include(\"${VITASDK}/share/vita.cmake\" REQUIRED)\n";
		cmakelists += "set(VITA_APP_NAME \"";
		cmakelists += appName;
		cmakelists += "\")\n";
		cmakelists += "set(VITA_TITLEID \"";
		cmakelists += appTitleID;
		cmakelists += "\")\n";
		cmakelists += "set(VITA_VERSION \"01.00\")\n";			// Make user give version
		cmakelists += "set(CMAKE_C_FLAGS \"${CMAKE_C_FLAGS} -Wall -O3\")\nset(CMAKE_CXX_FLAGS \"${CMAKE_CXX_FLAGS} -O3 -std=gnu++11 -DGLM_ENABLE_EXPERIMENTAL -DGLM_FORCE_RADIANS -DGLM_FORCE_DEPTH_ZERO_TO_ONE -DVITA\")\nset(VITA_MKSFOEX_FLAGS \"${VITA_MKSFOEX_FLAGS} -d PARENTAL_LEVEL=1\")\n";
		cmakelists += "include_directories(\n\t../../../../\n\t../../../../Engine\n\t../../../../include/bullet\n)\n";
		cmakelists += "link_directories(\n\t${CMAKE_CURRENT_BINARY_DIR}\n\t../../../../\n../../../../vitalibs\n)\n";
		cmakelists += "add_executable(" + appNameNoSpaces + " main.cpp)\n";
		cmakelists += "target_link_libraries(" + appNameNoSpaces + "\n\tVitaEngine\n\tLua5_3_4\n\tBulletDynamics\n\tBulletCollision\n\tBulletSoftBody\n\tLinearMath\n\tSceLibKernel_stub\n\tSceGxm_stub\n\tSceDisplay_stub\n\tSceCtrl_stub\n)\n";
		cmakelists += "vita_create_self(" + appNameNoSpaces + ".self " + appNameNoSpaces + ")\n";
		cmakelists += "vita_create_vpk(" + appNameNoSpaces + ".vpk ${VITA_TITLEID} " + appNameNoSpaces + ".self VERSION ${VITA_VERSION} NAME ${VITA_APP_NAME} FILE sce_sys sce_sys\n";

		// Add used files
		cmakelists += "FILE Data/" + curProjectName + ".proj Data/Levels/" + curProjectName + '/' + curProjectName + ".proj\n";
		cmakelists += "FILE ../main.bin Data/Levels/" + curProjectName + "/main.bin\n";
		cmakelists += "FILE ../main.rendersettings Data/Levels/" + curProjectName + "/main.rendersettings\n";

		// Add common scripts
		cmakelists += "FILE ../../../Resources/Scripts Data/Resources/Scripts\n";

		// Add font
		const Font &font = game->GetRenderingPath()->GetFont();
		cmakelists += "FILE ../../../../" + font.GetFontPath() + ' ' + font.GetFontPath() + '\n';
		cmakelists += "FILE ../../../../" + font.GetFontAtals()->GetPath() + ' ' + font.GetFontAtals()->GetPath() + '\n';

		const std::map<unsigned int, Model*> &uniqueModels = game->GetModelManager().GetUniqueModels();

		for (auto it = uniqueModels.begin(); it != uniqueModels.end(); it++)
		{
			Model *m = it->second;

			const std::vector<MeshMaterial>& meshesAndMaterials = m->GetMeshesAndMaterials();

			// If meshes and materials is 0 then the model is missing, so skip it
			if (meshesAndMaterials.size() == 0)
				continue;

			const std::string &p = m->GetPath();

			// Add the model's path
			cmakelists += "FILE ../../../../" + p + ' ' + p + '\n';

			// Add the materials path		
			for (size_t i = 0; i < meshesAndMaterials.size(); i++)
			{
				const MeshMaterial &mm = meshesAndMaterials[i];

				cmakelists += "FILE ../../../../" + mm.mat->path + ' ' + mm.mat->path + "\n";

				for (size_t j = 0; j < mm.mat->textures.size(); j++)
				{
					Texture *t = mm.mat->textures[j];
					cmakelists += "FILE ../../../../" + t->GetPath() + ' ' + t->GetPath() + "\n";
				}			
			}
		}

		// Add primitive models materials and textures
		const std::vector<ModelInstance> &models = game->GetModelManager().GetModels();
		for (size_t i = 0; i < models.size(); i++)
		{
			Model *m = models[i].model;

			if (m->GetType() == ModelType::PRIMITIVE_CUBE || m->GetType() == ModelType::PRIMITIVE_SPHERE)
			{
				const std::vector<MeshMaterial> &meshesAndMaterials = m->GetMeshesAndMaterials();
				for (size_t i = 0; i < meshesAndMaterials.size(); i++)
				{
					const MeshMaterial &mm = meshesAndMaterials[i];

					cmakelists += "FILE ../../../../" + mm.mat->path + ' ' + mm.mat->path + "\n";

					for (size_t j = 0; j < mm.mat->textures.size(); j++)
					{
						Texture *t = mm.mat->textures[j];
						cmakelists += "FILE ../../../../" + t->GetPath() + ' ' + t->GetPath() + "\n";
					}
				}
			}
		}


		// Add scripts
		const std::vector<ScriptInstance> &scripts = game->GetScriptManager().GetScripts();
		for (size_t i = 0; i < scripts.size(); i++)
		{
			const ScriptInstance& si = scripts[i];

			if (si.s)
			{
				const std::string& p = si.s->GetPath();

				cmakelists += "FILE ../../../../" + p + ' ' + p + '\n';
			}		
		}

		// Compile shaders if they have been modified
		// Check when the shader files have been written and if the date is more recent then the compiled one, then compile again

		const std::map<unsigned int, MaterialRefInfo> &uniqueMaterials = ResourcesLoader::GetMaterials();

		// Materials for the Vita are in Data/Materials/Vita for now
		cmakelists += "FILE ../../../Materials/Vita Data/Materials\n";
		// We also add the whole Shaders/GXM/compiled folder
		cmakelists += "FILE ../../../Shaders/GXM/compiled Data/Shaders/GXM\n";
		cmakelists += ")\n";

		std::ofstream file = std::ofstream(folderPath + "/CMakeLists.txt");
		if (file.is_open())
		{
			file << cmakelists;
		}
		file.close();

		std::ofstream settings = std::ofstream(folderPath + "/settings.txt");
		if (settings.is_open())
		{
			settings << "appName=" << appName << '\n';
			settings << "appTitleID=" << appTitleID << '\n';
		}
		settings.close();

		std::ofstream main = std::ofstream(folderPath + "/main.cpp");
		if (main.is_open())
		{
			main << "#include \"PSVitaApplication.h\"\n\
			class MyApplication : public Engine::PSVitaApplication\
			{public:\
			bool Init(){\
			if (!PSVitaApplication::Init())\
			return false;\
			if (!game.LoadProject(\"" + curProjectName + "\"))\
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
		main.close();

#ifdef _WIN32
		std::ofstream buildFile(folderPath + "/build.bat");
		if (buildFile.is_open())
		{
			buildFile << "mkdir Data\n";
			buildFile << "xcopy /k/c/y \"..\\" + curProjectName + ".proj\" \"Data\"\n";				// Copy the project file, making sure the previous dir is specified as ..\ instead of ../ otherwise xcopy won't copy
			// No longer necessary to change line endings as the project file is longer a text file
			//buildFile << "sed -i 's/\\r$//' Data/" + curProjectName + ".proj\n";					// Change the line endings. Should just save them all with linux line endings... Notepad++ is able to open and edit them anyway
			buildFile << "xcopy /s/h/e/k/c/y \"../../../Vita\" \".\"\n";
			buildFile << "cmake -G \"Unix Makefiles\" .\n";
			buildFile << "make";
		}
		buildFile.close();
#elif __linux__
		std::ofstream buildFile(folderPath + "/build.sh");
		if (buildFile.is_open())
		{
			//buildFile << "mkdir Data\n";
			buildFile << "cp -avr ../../../Vita .";
			buildFile << "cmake -G \"Unix Makefiles\" .\n";
			buildFile << "make";
		}
		buildFile.close();
#endif
		std::string cmd = "cd " + folderPath + " & build.bat";
		if (std::system(cmd.c_str()) != 0)
		{
			Log::Print(LogLevel::LEVEL_ERROR, "build.bat failed! Is cmake installed?\n");
			return;
		}

		Log::Print(LogLevel::LEVEL_INFO, "Project compiled for the PS Vita\n");
	}
}
