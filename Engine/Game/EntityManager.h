#pragma once

#include "Program\Serializer.h"

#include <limits>
#include <functional>
#include <vector>
#include <stack>

namespace Engine
{
	struct Entity
	{
		unsigned int id;

		bool IsValid() const { return id != std::numeric_limits<unsigned int>::max(); }
	};

	class EntityManager
	{
	public:
		EntityManager();
		~EntityManager();

		Entity Create();
		Entity Duplicate(Entity e);
		void Destroy(Entity e);

		void AddComponentDestroyCallback(const std::function<void(Entity)> &callback);
		void AddComponentDuplicateCallback(const std::function<void(Entity, Entity)> &callback);

		void Serialize(Serializer &s);
		void Deserialize(Serializer &s);

	private:
		Entity nextEntity;
		std::vector<std::function<void(Entity)>> destroyCallbacks;
		std::vector<std::function<void(Entity, Entity)>> duplicateEntityCallbacks;
		std::stack<unsigned int> freeIndices;
	};
}
