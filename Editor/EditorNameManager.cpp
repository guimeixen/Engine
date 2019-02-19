#include "EditorNameManager.h"

void EditorNameManager::SetName(Engine::Entity e, const char *name)
{
	// Just update the name if we already have this entity already has a name
	if (map.find(e.id) != map.end())
	{
		EditorName &en = names[map[e.id]];
		strcpy(en.name, name);
	}
	else
	{
		EditorName en = {};
		en.e = e;
		strcpy(en.name, name);

		if (counter < names.size())
		{
			names[counter] = en;
			map[e.id] = counter;
		}
		else
		{
			names.push_back(en);
			map[e.id] = names.size() - 1;
		}
		
		counter++;
	}
}

void EditorNameManager::DuplicateEditorName(Engine::Entity e, Engine::Entity newE)
{
	if (HasName(e) == false)
		return;

	EditorName en;
	en.e = newE;
	strcpy(en.name, "Duplicated entity");

	if (counter < names.size())
	{
		names[counter] = en;
		map[newE.id] = counter;
	}
	else
	{
		names.push_back(en);
		map[newE.id] = names.size() - 1;
	}

	counter++;
}

const char *EditorNameManager::GetName(Engine::Entity e) const
{
	return names[map.at(e.id)].name;
}

const EditorName &EditorNameManager::GetEditorName(Engine::Entity e) const
{
	return names[map.at(e.id)];
}

void EditorNameManager::RemoveName(Engine::Entity e)
{
	if (map.find(e.id) != map.end())
	{
		unsigned int index = map.at(e.id);

		EditorName temp = names[index];
		EditorName last = names[names.size() - 1];
		names[names.size() - 1] = temp;
		names[index] = last;

		map[last.e.id] = index;
		map.erase(e.id);

		counter--;
	}
}

bool EditorNameManager::HasName(Engine::Entity e) const
{
	return map.find(e.id) != map.end();
}

void EditorNameManager::Serialize(Engine::Serializer &s) const
{
	s.Write(counter);
	for (unsigned int i = 0; i < counter; i++)
	{
		const EditorName &en = names[i];
		s.Write(en.e.id);
		s.Write(en.name);
	}
}

void EditorNameManager::Deserialize(Engine::Serializer &s)
{
	s.Read(counter);
	names.resize(counter);
	for (unsigned int i = 0; i < counter; i++)
	{
		EditorName en = {};
		s.Read(en.e.id);
		s.Read(en.name);
		names[i] = en;
		map[en.e.id] = i;
	}
}
