#pragma once

#include "Game/EntityManager.h"
#include "Program/Serializer.h"

#include "include/lua/lua.hpp"
#include "include/LuaBridge/LuaBridge.h"

#include <string>
#include <vector>
#include <map>

namespace Engine
{
	enum ScriptFunctionID
	{
		OnAddEditorProperty = 0,
		OnInit,
		OnUpdate,
		OnEvent,
		OnRender,
		OnDamage,
		OnTriggerEnter,
		OnTriggerStay,
		OnTriggerExit,
		OnResize,
		OnTargetSeen,
		OnTargetInRange,
		OnButtonPressed
	};

	class Widget;
	class Game;

	struct ScriptProperty
	{
		std::string name;
		Entity e;
	};

	class Script
	{
	private:
		friend class ScriptManager;
	public:
		Script(lua_State *L, const std::string &name, const std::string &path, const luabridge::LuaRef &table);

		void CallOnAddEditorProperty(Entity e);
		void CallOnInit(Entity e);
		void CallOnUpdate(Entity e, float dt);
		void CallOnEvent(Entity e, int id);
		void CallOnTriggerEnter(Entity e);
		void CallOnTriggerStay(Entity e);
		void CallOnTriggerExit(Entity e);
		void CallOnResize(int width, int height);
		void CallOnTargetSeen(Entity target);
		void CallOnTargetInRange(Entity target);
		void CallOnButtonPressed();

		void SetFunction(ScriptFunctionID id, luabridge::LuaRef *func);

		void AddProperty(const std::string &name);
		void SetProperty(const std::string &name, Entity e);
		void ReloadProperties();
		void RemovedUnusedProperties();
		void Dispose();

		const std::string &GetName() const { return name; }
		const std::string &GetPath() const { return path; }

		bool HasTriggerFunctions() const { return hasTriggerFunctions; }

		const std::vector<ScriptProperty> &GetProperties() const { return properties; }

		void Serialize(Serializer &s);
		void Deserialize(Serializer &s);

		// Script functions
		luabridge::LuaRef GetEnvironment();

	protected:
		std::map<ScriptFunctionID, luabridge::LuaRef*> functions;

	private:
		lua_State *L;
		std::string name;
		std::string path;
		bool hasTriggerFunctions = false;
		std::vector<ScriptProperty> properties;
		std::vector<int> newProperties;
		luabridge::LuaRef table;
	};
}
