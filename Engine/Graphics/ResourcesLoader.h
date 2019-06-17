#pragma once

#include "Game/ComponentManagers/ScriptManager.h"
#include "VertexTypes.h"

#include <map>

namespace Engine
{
	class Shader;
	class Material;
	class Renderer;
	struct MaterialInstance;

	struct MaterialRefInfo
	{
		Material *mat;
		unsigned int ocurrences;
	};

	class ResourcesLoader
	{
	public:
		static void Clean(); 

		static Material*			LoadMaterial(Renderer *renderer, const std::string &path, ScriptManager &scriptManager, const std::vector<VertexInputDesc> &descs);
		static Material*			LoadMaterial(Renderer *renderer, const std::string &path, const std::string &defines, ScriptManager &scriptManager, const std::vector<VertexInputDesc> &descs);
		static Material*			CreateEmptyMaterial(Renderer *renderer, ScriptManager &scriptManager, const std::vector<VertexInputDesc> &descs);

		static bool RemoveMaterial(Material *m);

		static std::map<unsigned int, MaterialRefInfo> &GetMaterials() { return materials; }

	private:
		static std::map<unsigned int, MaterialRefInfo> materials;
	};
}
