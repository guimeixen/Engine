#pragma once

#include "Graphics\RendererStructs.h"
#include "Program\Serializer.h"

#include <vector>

namespace Engine
{
	class Transform;
	class Object;
	class Model;
	class Game;
	class Camera;

	class SceneManager
	{
	public:
		void SavePrefab(Object *obj, const std::string &path);
		Object *LoadPrefab(const std::string &path);
	private:
		void SavePrefabRecursive(Transform *t, Serializer &s);

	private:
		Game *game;
		std::vector<Object*> objects;
	};
}
