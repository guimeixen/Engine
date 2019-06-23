#include "ResourcesLoader.h"

#include "Program/StringID.h"
#include "Material.h"
#include "Renderer.h"

#include <iostream>

namespace Engine
{
	std::map<unsigned int, MaterialRefInfo> ResourcesLoader::materials;

	void ResourcesLoader::Clean()
	{
		for (auto it = materials.begin(); it != materials.end(); it++)
		{
			delete it->second.mat;
		}
	}

	Material *ResourcesLoader::LoadMaterial(Renderer *renderer, const std::string &path, ScriptManager &scriptManager, const std::vector<VertexInputDesc> &descs)
	{
		return LoadMaterial(renderer, path, "", scriptManager, descs);
	}

	Material *ResourcesLoader::LoadMaterial(Renderer *renderer, const std::string &path, const std::string &defines, ScriptManager &scriptManager, const std::vector<VertexInputDesc> &descs)
	{
		unsigned int id = SID(path + defines);	// Add the defines to the material path because one mat.instance could define instancing but the once the base mat is requested again from
												// another mat.instance without instancing, LoadMaterial() would return the first one because it has the same name and has already been loaded
												// which would lead to the shaders requested to be incorrect
		if (materials.find(id) != materials.end())
		{
			materials[id].ocurrences++;
			return materials[id].mat;
		}

		MaterialRefInfo info = {};
		info.ocurrences = 1;

		Material *mat = new Material(renderer, path, defines + renderer->GetGlobalDefines(), scriptManager, descs);
		info.mat = mat;

		materials[id] = info;

		return mat;
	}

	Material *ResourcesLoader::CreateEmptyMaterial(Renderer *renderer, ScriptManager &scriptManager, const std::vector<VertexInputDesc> &descs)
	{
		return LoadMaterial(renderer, "Data/Resources/Materials/default_mat", scriptManager, descs);
	}

	bool ResourcesLoader::RemoveMaterial(Material *m)
	{
		for (auto it = materials.begin(); it != materials.end(); it++)
		{
			if (it->second.mat == m)
			{
				if (it->second.ocurrences == 1)
				{
					materials.erase(it);
					delete m;
					m = nullptr;
					return true;
				}
				else
				{
					it->second.ocurrences--;
					return false;
				}
			}
		}

		return false;
	}
}