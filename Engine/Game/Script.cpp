#include "Script.h"

#include "Game\UI\StaticText.h"
#include "Game\UI\Button.h"
#include "Game\UI\Image.h"
#include "Game\UI\EditText.h"
#include "Game\Game.h"
#include "Game\UI\UIManager.h"

#include <iostream>

namespace Engine
{
	Script::Script(lua_State *L, const std::string &name, const std::string &path, const luabridge::LuaRef &table) : table(table)
	{
		this->L = L;
		this->name = name;
		this->path = path;
	}

	Script::~Script()
	{
	}

	void Script::CallOnAddEditorProperty(Entity e)
	{
		luabridge::LuaRef *ref = functions[OnAddEditorProperty];

		if (ref)
		{
			try
			{
				//(*ref)();
				ref->colon(table, e);
				//(*ref).colon();
			}
			catch (luabridge::LuaException const& e)
			{
				std::cout << "OnAddEditorProperty LuaException: " << e.what() << "\n";
			}
		}
	}

	void Script::CallOnInit(Entity e)
	{
		luabridge::LuaRef *ref = functions[OnInit];

		if (ref)
		{
			try
			{
				//(*ref)(e);
				ref->colon(table, e);
			}
			catch (luabridge::LuaException const& e)
			{
				std::cout << "OnInit LuaException: " << e.what() << "\n";
			}
		}
	}

	void Script::CallOnUpdate(Entity e, float dt)
	{
		luabridge::LuaRef *ref = functions[OnUpdate];

		if (ref)
		{
			try
			{
				//(*ref)(e, dt);
				ref->colon(table, e, dt);
			}
			catch (luabridge::LuaException const& e)
			{
				std::cout << path << '\n';
				std::cout << "OnUpdate LuaException: " << e.what() << "\n";
			}
		}
	}

	void Script::CallOnEvent(Object *obj, int id)
	{
		try
		{
			(*functions[OnEvent])(obj, id);
		}
		catch (luabridge::LuaException const& e)
		{
			std::cout << "OnEvent LuaException: " << e.what() << "\n";
		}
	}

	void Script::CallOnTriggerEnter(Object *obj)
	{
		luabridge::LuaRef *ref = functions[OnTriggerEnter];

		if (ref)
		{
			try
			{
				(*ref)(obj);
			}
			catch (luabridge::LuaException const& e)
			{
				std::cout << "OnTriggerEnter LuaException: " << e.what() << "\n";
			}
		}
	}

	void Script::CallOnTriggerStay(Object *obj)
	{
		if (functions[OnTriggerStay])
		{
			try
			{
				(*functions[OnTriggerStay])(obj);
			}
			catch (luabridge::LuaException const& e)
			{
				std::cout << "OnTriggerStay LuaException: " << e.what() << "\n";
			}
		}
	}

	void Script::CallOnTriggerExit(Object *obj)
	{
		if (functions[OnTriggerExit])
		{
			try
			{
				(*functions[OnTriggerExit])(obj);
			}
			catch (luabridge::LuaException const& e)
			{
				std::cout << "OnTriggerExit LuaException: " << e.what() << "\n";
			}
		}
	}

	void Script::CallOnResize(int width, int height)
	{
		luabridge::LuaRef *ref = functions[OnResize];

		if (ref)
		{
			try
			{
				(*ref)(width, height);
			}
			catch (luabridge::LuaException const& e)
			{
				std::cout << "OnResize LuaException: " << e.what() << "\n";
			}
		}
	}

	void Script::CallOnTargetSeen(Object *target)
	{
		luabridge::LuaRef *ref = functions[OnTargetSeen];

		if (ref)
		{
			try
			{
				(*ref)(target);
			}
			catch (luabridge::LuaException const& e)
			{
				std::cout << "OnTargetInRange LuaException: " << e.what() << "\n";
			}
		}
	}

	void Script::CallOnTargetInRange(Object *target)
	{
		luabridge::LuaRef *ref = functions[OnTargetInRange];

		if (ref)
		{
			try
			{
				(*ref)(target);
			}
			catch (luabridge::LuaException const& e)
			{
				std::cout << path << '\n';
				std::cout << "OnTargetInRange LuaException: " << e.what() << "\n";
			}
		}
	}

	void Script::CallOnButtonPressed()
	{
		if (functions[OnButtonPressed])
		{
			try
			{
				(*functions[OnButtonPressed])();
			}
			catch (luabridge::LuaException const& e)
			{
				std::cout << "OnButtonPressed LuaException: " << e.what() << "\n";
			}
		}
	}

	void Script::SetFunction(ScriptFunctionID id, luabridge::LuaRef *func)
	{
		functions[id] = func;

		switch (id)
		{
		case OnTriggerEnter:
			hasTriggerFunctions = true;
			break;
		case OnTriggerStay:
			hasTriggerFunctions = true;
			break;
		case OnTriggerExit:
			hasTriggerFunctions = true;
			break;
		}
	}

	void Script::AddProperty(const std::string &name)
	{
		for (size_t i = 0; i < properties.size(); i++)
		{
			if (properties[i].name == name)		// Don't repeat a property if the name is the same as the parameters
			{
				newProperties.push_back(i);
				return;
			}
		}

		ScriptProperty p = {};
		p.name = name;
		p.e = { std::numeric_limits<unsigned int>::max() };

		properties.push_back(p);
		newProperties.push_back(properties.size() - 1);		// Push back the index of the last properties element
	}

	void Script::SetProperty(const std::string &name, Entity e)
	{
		if (!e.IsValid())
			return;

		for (size_t i = 0; i < properties.size(); i++)
		{
			if (properties[i].name == name)
			{
				properties[i].e = e;
				break;
			}
		}
	}

	void Script::ReloadProperties()
	{
		for (size_t i = 0; i < properties.size(); i++)
		{
			/*lua_getglobal(L, "pickup");
			lua_pushstring(L, "playerr");
			luabridge::push(L, properties[i].obj);
			lua_settable(L, -3);
			lua_setglobal(L, "pickup");*/

			/*lua_getglobal(L, name.c_str());		// Get the table
			lua_pushstring(L, properties[i].name.c_str());

			if (properties[i].e.IsValid())
				luabridge::push(L, properties[i].e);			
			else
				lua_pushnil(L);

			lua_settable(L, -3);
			lua_setglobal(L, name.c_str());		// Set the table*/

			table[properties[i].name] = properties[i].e;
		}
	}

	void Script::RemovedUnusedProperties()
	{
		bool found = false;
		for (size_t i = 0; i < properties.size(); i++)
		{
			found = false;
			for (size_t j = 0; j < newProperties.size(); j++)
			{
				if (newProperties[j] == i)
				{
					found = true;
					break;
				}
			}
			if (!found)
				properties.erase(properties.begin() + i);
		}
		newProperties.clear();
	}

	void Script::Dispose()
	{
		for (auto it = functions.begin(); it != functions.end(); it++)
		{
			delete it->second;
			it->second = nullptr;
		}
		functions.clear();
	}

	void Script::Serialize(Serializer &s)
	{
		s.Write(path);
		s.Write(properties.size());

		for (size_t i = 0; i < properties.size(); i++)
		{
			const ScriptProperty &sp = properties[i];
			s.Write(sp.name);
			s.Write(sp.e.id);
		}
	}

	void Script::Deserialize(Serializer &s)
	{
		// We don't read path because it's read by who calls Deserialize so it can load the script

		unsigned int propertiesCount = 0;
		s.Read(propertiesCount);

		std::string name;
		for (size_t i = 0; i < propertiesCount; i++)
		{
			ScriptProperty prop;
			s.Read(prop.name);
			s.Read(prop.e.id);

			// Deserialize might be called when we load a project or stop the game in the editor. If it's the latter, we already have the properties loaded
			// In that case, we don't need to push back and just need to read what we serialized as is done above

			if (i >= properties.size())
				properties.push_back(prop);
		}
	}

	luabridge::LuaRef Script::GetEnvironment()
	{
		return table;
	}
}
