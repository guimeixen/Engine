#pragma once

#include "Game\Script.h"
#include "Game\EntityManager.h"

#include <unordered_map>

namespace Engine
{
	class Game;

	struct ScriptInstance
	{
		Entity e;
		Script *s;
	};

	class ScriptManager
	{
	public:
		ScriptManager();
		~ScriptManager();

		void Init(Game *game);
		void Play();
		void UpdateInGame(float dt);
		void ReloadAll();
		void ReloadScripts();
		void ReloadProperties();
		void ReloadFile(Script *script);
		void PartialDispose();
		void Dispose();

		Script *AddScript(Entity e, const std::string &fileName);
		void DuplicateScript(Entity e, Entity newE);
		void RemoveScript(Entity e);
		void ExecuteFile(const std::string &fileName);
		void CallFunction(const std::string &functionName);

		bool HasScript(Entity e) const;
		Script *GetScript(Entity e) const;

		void Serialize(Serializer &s) const;
		void Deserialize(Serializer &s, bool reload = false);

		template <class T>
		inline void Call(const std::string &tableName, const std::string &functionName, T t)
		{
			lua_getglobal(L, tableName.c_str());		// Get table into the stack
			int table = lua_gettop(L);							// Get the table index which is at the top of the stack
			lua_getfield(L, table, functionName.c_str());		// Get the function into the stack as well
			luabridge::push(L, t);							// Push the parameter
			lua_pcall(L, 1, 0, 0);
			lua_settop(L, table - 1);				// If we have the table index then it's easy to clean the stack
		}

		template <class T>
		inline void SetGlobal(const std::string &name, T t)		// name should be equal to the one registered with luabridge
		{
			luabridge::push(L, t);
			lua_setglobal(L, name.c_str());
		}

		std::map<std::string, luabridge::LuaRef> GetKeyValueMap(const luabridge::LuaRef &table);

		lua_State *GetLuaState() const { return L; }
		const std::vector<ScriptInstance> &GetScripts() const { return scripts; }

	private:
		void ReadTable(Script *script, const luabridge::LuaRef &table);
		Script *LoadScript(const std::string &fileName);

	private:
		lua_State *L;

		std::vector<ScriptInstance> scripts;
		std::unordered_map<unsigned int, unsigned int> map;
		unsigned int usedScripts = 0;
	};
}
