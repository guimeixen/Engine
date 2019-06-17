#include "EntityManager.h"

namespace Engine
{
	EntityManager::EntityManager()
	{
		nextEntity = {};
	}

	Entity EntityManager::Create()
	{	
		if (freeIndices.size() > 0)
		{
			Entity newEntity = { freeIndices.top() };
			freeIndices.pop();
			return newEntity;
		}
		else
		{
			Entity newEntity = nextEntity;
			nextEntity.id++;
			return newEntity;
		}
	}

	Entity EntityManager::Duplicate(Entity e)
	{
		Entity duplicatedEntity = Create();

		for (size_t i = 0; i < duplicateEntityCallbacks.size(); i++)
		{
			duplicateEntityCallbacks[i](e, duplicatedEntity);
		}

		return duplicatedEntity;
	}

	void EntityManager::Destroy(Entity e)
	{
		if (e.IsValid() == false)
			return;

		freeIndices.push(e.id);

		// Call destroy on the component managers using callbacks
		for (size_t i = 0; i < destroyCallbacks.size(); i++)
		{
			destroyCallbacks[i](e);
		}
	}

	void EntityManager::AddComponentDestroyCallback(const std::function<void(Entity)> &callback)
	{
		destroyCallbacks.push_back(callback);
	}

	void EntityManager::AddComponentDuplicateCallback(const std::function<void(Entity, Entity)> &callback)
	{
		duplicateEntityCallbacks.push_back(callback);
	}

	void EntityManager::Serialize(Serializer &s)
	{
		s.Write(nextEntity.id);
		s.Write(freeIndices.size());
		for (size_t i = 0; i < freeIndices.size(); i++)
		{
			s.Write(freeIndices.top());
			freeIndices.pop();
		}
	}

	void EntityManager::Deserialize(Serializer &s)
	{
		s.Read(nextEntity.id);
		size_t size = 0;
		s.Read(size);
		unsigned int temp;
		for (size_t i = 0; i < size; i++)
		{
			s.Read(temp);
			freeIndices.push(temp);			// The indices will be flipped, but it is not a problem. We're just interested in the value of the indices and not their order.
		}
	}
}
