#include "EntityManager.h"

#include "Program/Log.h"

#include <algorithm>

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

	void EntityManager::SetEnabled(Entity e, bool enable)
	{
		// Used so we don't call the setEnabled functions to enable an entity when it's already enabled, the same for disabling
		bool dontRepeat = false;

		if (enable)
		{
			// If we have the entity in the disabled entities list then remove it
			auto it = std::find(disabledEntities.begin(), disabledEntities.end(), e.id);
			if (it != disabledEntities.end())
			{
				disabledEntities.erase(it);
			}
			else
			{
				dontRepeat = true;				// If we don't have the entity in the list then it is enabled, so don't call the setEnabled functions
			}
		}
		else
		{
			// If we don't have the entity in the disabled entites list then add it
			if (std::find(disabledEntities.begin(), disabledEntities.end(), e.id) == disabledEntities.end())
			{
				disabledEntities.push_back(e.id);
			}
			else
			{
				dontRepeat = true;				// If we already have the entity in the list then don't call the setEnabled functions again
			}
		}

		if (!dontRepeat)
		{
			for (size_t i = 0; i < setEnabledEntityCallbacks.size(); i++)
			{
				setEnabledEntityCallbacks[i](e, enable);
			}
		}
	}

	bool EntityManager::IsEntityEnabled(Entity e)
	{
		auto it = std::find(disabledEntities.begin(), disabledEntities.end(), e.id);

		return it == disabledEntities.end();
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

	void EntityManager::AddComponentSetEnabledCallback(const std::function<void(Entity, bool)> &callback)
	{
		setEnabledEntityCallbacks.push_back(callback);
	}

	void EntityManager::Serialize(Serializer &s)
	{
		s.Write(nextEntity.id);
		s.Write(static_cast<unsigned int>(freeIndices.size()));
		for (size_t i = 0; i < freeIndices.size(); i++)
		{
			s.Write(freeIndices.top());
			freeIndices.pop();
		}

		s.Write((unsigned int)disabledEntities.size());
		for (size_t i = 0; i < disabledEntities.size(); i++)
		{
			s.Write(disabledEntities[i]);
		}
	}

	void EntityManager::Deserialize(Serializer &s)
	{
		Log::Print(LogLevel::LEVEL_INFO, "Deserializing entity manager\n");

		s.Read(nextEntity.id);
		unsigned int size = 0;
		s.Read(size);
		unsigned int temp;
		for (unsigned int i = 0; i < size; i++)
		{
			s.Read(temp);
			freeIndices.push(temp);			// The indices will be flipped, but it is not a problem. We're just interested in the value of the indices and not their order.
		}

		unsigned int disabledEntitiesCount;
		s.Read(disabledEntitiesCount);

		disabledEntities.resize(disabledEntitiesCount);
		for (size_t i = 0; i < disabledEntities.size(); i++)
		{
			s.Read(disabledEntities[i]);
		}
	}
}
