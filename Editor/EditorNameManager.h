#pragma once

#include "Game/EntityManager.h"

#include <vector>
#include <unordered_map>

struct EditorName
{
	Engine::Entity e;
	char name[128];
};

class EditorNameManager
{
public:
	void SetName(Engine::Entity e, const char *name);
	void DuplicateEditorName(Engine::Entity e, Engine::Entity newE);
	const char *GetName(Engine::Entity e) const;
	const EditorName &GetEditorName(Engine::Entity e) const;
	void RemoveName(Engine::Entity e);
	bool HasName(Engine::Entity e) const;

	const std::vector<EditorName> &GetNames() const { return names; }
	unsigned int GetNameCount() const { return counter; }

	void Serialize(Engine::Serializer &s) const;
	void Deserialize(Engine::Serializer &s);

private:
	unsigned int counter = 0;
	std::vector<EditorName> names;
	std::unordered_map<unsigned int, unsigned int> map;
};

