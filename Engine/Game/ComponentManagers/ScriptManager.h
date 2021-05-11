#pragma once

#include "Game/Script.h"

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
		void Init(Game *game);
		void Play();
		void UpdateInGame(float dt);
		void ReloadAll();
		void ReloadScripts();
		void ReloadProperties();
		void ReloadFile(const ScriptInstance &si);
		void PartialDispose();
		void Dispose();

		Script *AddScript(Entity e, const std::string &fileName);
		void DuplicateScript(Entity e, Entity newE);
		void LoadFromPrefab(Serializer &s, Entity e);
		void RemoveScript(Entity e);
		void SetScriptEnabled(Entity e, bool enable);
		void ExecuteFile(const std::string &fileName);
		void CallFunction(const std::string &functionName);
		std::string GetScriptErrorStr(Entity e);

		bool HasScript(Entity e) const;
		Script *GetScript(Entity e) const;
		luabridge::LuaRef GetScriptEnvironment(Entity e) const;

		void Serialize(Serializer &s, bool playMode = false) const;
		void Deserialize(Serializer &s, bool playMode = false);

		template <class T>
		inline void Call(const std::string &tableName, const std::string &functionName, T t)
		{
			lua_getglobal(L, tableName.c_str());				// Get table into the stack
			int table = lua_gettop(L);							// Get the table index which is at the top of the stack
			lua_getfield(L, table, functionName.c_str());		// Get the function into the stack as well
			luabridge::push(L, t);								// Push the parameter
			lua_pcall(L, 1, 0, 0);
			lua_settop(L, table - 1);							// If we have the table index then it's easy to clean the stack
		}

		template <class T>
		inline void SetGlobal(const std::string &name, T t)		// name should be equal to the one registered with luabridge
		{
			luabridge::push(L, t);
			lua_setglobal(L, name.c_str());
		}

		std::unordered_map<std::string, luabridge::LuaRef> GetKeyValueMap(const luabridge::LuaRef &table);

		lua_State *GetLuaState() const { return L; }
		const std::vector<ScriptInstance> &GetScripts() const { return scripts; }

	private:
		Script *LoadScript(Entity e, const std::string &fileName);
		void ExecuteString(const char *str);

		void InsertScriptInstance(const ScriptInstance &si);

	private:
		lua_State *L;
		Game *game;

		std::vector<ScriptInstance> scripts;
		std::unordered_map<unsigned int, unsigned int> map;
		unsigned int usedScripts = 0;
		unsigned int disabledScripts = 0;

		std::unordered_map<unsigned int, std::string> scriptErrorsMap;
	};
}
