#include "ScriptManager.h"

#include "Program/Input.h"
#include "Program/Log.h"
#include "Program/FileManager.h"
#ifndef VITA
#include "Program/Window.h"
#endif

#include "Game/UI/UIManager.h"
#include "Game/UI/StaticText.h"
#include "Game/UI/Button.h"
#include "Game/UI/Image.h"
#include "Game/UI/EditText.h"
#include "Game/Game.h"

#include "Physics/RigidBody.h"
#include "Physics/Collider.h"

#include "Graphics/Texture.h"
#include "Graphics/ParticleSystem.h"
#include "Graphics/Animation/AnimatedModel.h"
#include "Graphics/Terrain/Terrain.h"
#include "Graphics/Effects/MainView.h"
#include "Graphics/Effects/TimeOfDayManager.h"

#include "Game/ComponentManagers/SoundManager.h"

#ifdef EDITOR
#include "../Editor/EditorManager.h"
#endif

#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>

namespace Engine
{
	void ScriptManager::Init(Game *game)
	{
		this->game = game;

		L = luaL_newstate();
		luaL_openlibs(L);

#ifdef VITA
		// Add app0 as path for searching modules
		ExecuteString("package.path = package.path .. \";app0:Data/Resources/Scripts/?.lua\"");
#else
		ExecuteString("package.path = package.path .. \";Data/Resources/Scripts/?.lua\"");
#endif

		Log::Print(LogLevel::LEVEL_INFO, "Init Lua\n");

		luabridge::getGlobalNamespace(L)

			.beginClass<Entity>("Entity")
			.addConstructor<void(*)()>()
			.addData("id", &Entity::id, true)
			.endClass()

			.beginClass<glm::vec2>("vec2")
			.addConstructor<void(*)(float, float)>()
			.addData("x", &glm::vec2::x, true)
			.addData("y", &glm::vec2::y, true)
			.endClass()

			.beginClass<glm::vec3>("vec3")
			.addConstructor<void(*)(float, float, float)>()
			.addData("x", &glm::vec3::x, true)
			.addData("y", &glm::vec3::y, true)
			.addData("z", &glm::vec3::z, true)
			.endClass()

			.beginClass<glm::vec4>("vec4")
			.addConstructor<void(*)(float, float, float, float)>()
			.addData("x", &glm::vec4::x, true)
			.addData("y", &glm::vec4::y, true)
			.addData("z", &glm::vec4::z, true)
			.addData("w", &glm::vec4::w, true)
			.endClass()

			.beginClass<RaycastResult>("RaycastResult")
			.addConstructor<void(*)()>()
			.addData("e", &RaycastResult::e, true)
			.addData("hit", &RaycastResult::hit, true)
			.addData("hitDistance", &RaycastResult::hitDistance, true)
			.endClass()

			.beginClass<TransformManager>("Transform")
			.addFunction("setLocalPos", &TransformManager::SetLocalPosition)
			.addFunction("setLocalRot", &TransformManager::SetLocalRotation)
			.addFunction("setLocalRotEuler", &TransformManager::SetLocalRotationEuler)
			.addFunction("setLocalScale", &TransformManager::SetLocalScale)
			.addFunction("getLocalPos", &TransformManager::GetLocalPosition)
			.addFunction("getLocalRot", &TransformManager::GetLocalRotation)
			.addFunction("getLocalScale", &TransformManager::GetLocalScale)
			.addFunction("getWorldPos", &TransformManager::GetWorldPosition)
			.addFunction("getParent", &TransformManager::GetParent)
			.addFunction("rotate", &TransformManager::Rotate)
			.endClass()

			.beginClass<PhysicsManager>("Physics")
			.addFunction("getRigidBody", &PhysicsManager::GetRigidBodySafe)
			.addFunction("getCollider", &PhysicsManager::GetColliderSafe)
			.addFunction("getTrigger", &PhysicsManager::GetTriggerSafe)
			.addFunction("raycast", &PhysicsManager::PerformRaycast)
			.endClass()

			.beginClass<ScriptManager>("ScriptManager")
			.addFunction("getScript", &ScriptManager::GetScriptEnvironment)
			.endClass()

			.beginClass<Collider>("Collider")
			.addFunction("getPos", &Collider::GetPosition)
			.addFunction("setEnabled", &Collider::SetEnabled)
			.endClass()

			.beginClass<RigidBody>("RigidBody")
			.addFunction("getPos", &RigidBody::GetPosition)
			.addFunction("setKinematic", &RigidBody::SetKinematic)
			.addFunction("getLinearVelocity", &RigidBody::GetLinearVelocity)
			.addFunction("setLinearVelocity", &RigidBody::SetLinearVelocity)
			.addFunction("setLinearFactor", &RigidBody::SetLinearFactor)
			.addFunction("applyForce", &RigidBody::ApplyForce)
			.addFunction("applyCentralImpulse", &RigidBody::ApplyCentralImpulse)
			.addFunction("disableDeactivation", &RigidBody::DisableDeactivation)
			.addFunction("activate", &RigidBody::Activate)
			.endClass()

			.beginClass<Widget>("Widget")
			.addFunction("setEnabled", &Widget::SetEnabled)
			.addFunction("getRectSize", &Widget::GetRectSize)
			.addFunction("setRectSize", &Widget::SetRectSize)
			.addFunction("setAlpha", &Widget::SetAlpha)
			.addStaticFunction("castToButton", &Widget::CastToButton)
			.endClass()

			.beginClass<Input>("Input")
			.addStaticFunction("isKeyPressed", &Input::IsKeyPressed)
			.addStaticFunction("wasKeyReleased", &Input::WasKeyReleased)
			.addStaticFunction("isMouseButtonDown", &Input::IsMouseButtonDown)
			.addStaticFunction("wasMouseButtonReleased", &Input::WasMouseButtonReleased)
			.addStaticFunction("isVitaButtonDown", &Input::IsVitaButtonDown)
			.addStaticFunction("getAxis", &Input::GetAxis)
			.addStaticFunction("getAction", &Input::GetAction)
			.endClass()

			.beginClass<Game>("Game")
			.addFunction("getMainCamera", &Game::GetMainCamera)
			.addFunction("getRenderer", &Game::GetRenderingPath)
			//.addFunction("getRenderer", &Game::GetRenderer)
			.addFunction("getLightManager", &Game::GetLightManager)
			.addFunction("getTerrain", &Game::GetTerrain)
			.addFunction("getUIManager", &Game::GetUIManager)
			.addFunction("getSoundManager", &Game::GetSoundManager)
			.addFunction("loadScene", &Game::SetSceneScript)
			.addFunction("shutdown", &Game::Shutdown)
			.addFunction("play", &Game::Play)
			.addFunction("pause", &Game::Pause)
			.addFunction("setEntityEnabled", &Game::SetEntityEnabled)
			.endClass()

			/*.beginClass<MainView>("Renderer")
			.addFunction("getTodManager", &MainView::GetTimeOfDayManager)
			//.addFunction("spawnDecal", &MainView::SpawnDecal)
			.addFunction("getWidth", &MainView::GetWidth)
			.addFunction("getHeight", &MainView::GetHeight)
			.endClass()*/

			.beginClass<RenderingPath>("Renderer")
			.addFunction("getFont", &RenderingPath::GetFont)
			.endClass()

			.beginClass<Font>("Font")
			.addFunction("addText", &Font::AddText)
			.endClass()

			.beginClass<TimeOfDayManager>("TOD")
			.addFunction("setCurrentTime", &TimeOfDayManager::SetCurrentTime)
			.endClass()

			.beginClass<Terrain>("Terrain")
			.addFunction("getHeightAt", &Terrain::GetHeightAt)
			.addFunction("getNormalAtFast", &Terrain::GetNormalAtFast)
			.endClass()

			.beginClass<LightManager>("LightManager")
			.addFunction("toPointLight", &LightManager::CastToPointLight)
			.endClass()

			.beginClass<UIManager>("UI")
			.addFunction("showCursor", &UIManager::ShowCursor)
			.addFunction("getText", &UIManager::GetText)
			.addFunction("getButton", &UIManager::GetButton)
			.addFunction("getEditText", &UIManager::GetEditText)
			.addFunction("getImage", &UIManager::GetImage)
			.endClass()

			.beginClass<Texture>("Texture")			// Required to be able to load a texture through Lua
			.endClass()

			.beginClass<Camera>("Camera")
			//.addFunction("setPos", &Camera::SetPosition)
			.addFunction("getPos", &Camera::GetPositionScript)
			.addFunction("getForward", &Camera::GetFront)
			.addFunction("getRight", &Camera::GetRight)
			.addFunction("getViewportSize", &Camera::GetViewportSize)
			.addFunction("setPitch", &Camera::SetPitch)
			.addFunction("getPitch", &Camera::GetPitch)
			.addFunction("setYaw", &Camera::SetYaw)
			.addFunction("getYaw", &Camera::GetYaw)
			.addFunction("reset", &Camera::Reset)
			.addFunction("getSensitivity", &Camera::GetSensitivity)
			.addFunction("setSensitivity", &Camera::SetSensitivity)
			.endClass()

			.deriveClass<FPSCamera, Camera>("FPSCamera")
			.addFunction("setPos", &FPSCamera::SetPosition)
			.endClass()

			.beginClass<ParticleSystem>("ParticleSystem")
			.addFunction("stop", &ParticleSystem::Stop)
			.addFunction("play", &ParticleSystem::Play)
			.endClass()

#ifdef EDITOR
			.beginClass<EditorManager>("Editor")
			.addFunction("addEntityProperty", &EditorManager::AddEntityScriptProperty)
			.endClass()
#endif
			.beginClass<Engine::AnimatedModel>("AnimatedModel")
			.addFunction("stopAnim", &Engine::AnimatedModel::StopAnimation)
			.addFunction("playAnim", &Engine::AnimatedModel::PlayAnimation)
			.endClass()

			.deriveClass<StaticText, Widget>("Text")
			.addFunction("getText", &StaticText::GetText)
			.addFunction("setText", &StaticText::SetText)
			.endClass()

			.deriveClass<Button, Widget>("Button")
			.addFunction("getText", &Button::GetText)
			.addFunction("setText", &Button::SetText)
			.endClass()

			.deriveClass<Image, Widget>("Image")
			.endClass()

			.deriveClass<EditText, Widget>("EditText")
			.addFunction("getText", &EditText::GetText)
			.addFunction("setText", &EditText::SetText)
			.endClass()

			.beginClass<Light>("Light")
			.addData("intensity", &Light::intensity)
			.addData("color", &Light::color)
			.endClass()

			.deriveClass<PointLight, Light>("PointLight")
			.addData("position", &PointLight::position)
			.addData("radius", &PointLight::radius)
			.endClass()

			/*.beginClass<SoundManager>("SoundManager")
			.addFunction("loadSoundEffect", &SoundManager::LoadSoundEffect)
			.endClass()

			.beginClass<SoundSource>("SoundSource")
			.addFunction("play", &SoundSource::Play)
			.addFunction("stop", &SoundSource::Stop)
			.addFunction("setSound", &SoundSource::SetSound)
			.addFunction("setVolume", &SoundSource::SetVolume)
			.addFunction("setPitch", &SoundSource::SetPitch)
			.addFunction("isPlaying", &SoundSource::IsPlaying)
			.addFunction("getHandle", &SoundSource::GetSoundHandle)
			.endClass()

			.beginClass<FMOD::Sound>("FMODSound")
			.endClass()*/;

		Log::Print(LogLevel::LEVEL_INFO, "Init Lua Bridge\n");

		luabridge::push(L, game);
		lua_setglobal(L, "Game");

		luabridge::push(L, &game->GetTransformManager());
		lua_setglobal(L, "Transform");

		//luabridge::push(L, &game->GetModelManager());
		//lua_setglobal(L, "ModelManager");

		luabridge::push(L, &game->GetLightManager());
		lua_setglobal(L, "LightManager");

		//luabridge::push(L, &game->GetParticleManager());
		//lua_setglobal(L, "ParticleManager");

		luabridge::push(L, &game->GetPhysicsManager());
		lua_setglobal(L, "Physics");

		luabridge::push(L, this);
		lua_setglobal(L, "ScriptManager");

		/*luabridge::push(L, &game->GetSoundManager());
		lua_setglobal(L, "SoundManager");*/

		luabridge::push(L, &game->GetUIManager());
		lua_setglobal(L, "UI");

		Log::Print(LogLevel::LEVEL_INFO, "Added engine globals to Lua\n");
		Log::Print(LogLevel::LEVEL_INFO, "Init script manager\n");
	}

	void ScriptManager::Play()
	{
		const unsigned int numEnabledScripts = usedScripts - disabledScripts;
		for (unsigned int i = 0; i < numEnabledScripts; i++)
		{
			const ScriptInstance &si = scripts[i];
			si.s->CallOnInit(si.e);
		}
	}

	void ScriptManager::UpdateInGame(float dt)
	{
		const unsigned int numEnabledScripts = usedScripts - disabledScripts;
		for (unsigned int i = 0; i < numEnabledScripts; i++)
		{
			const ScriptInstance &si = scripts[i];
			si.s->CallOnUpdate(si.e, dt);
		}
	}

	void ScriptManager::ReloadAll()
	{
		Log::Print(LogLevel::LEVEL_INFO, "Reloading scripts...");

		for (size_t i = 0; i < usedScripts; i++)
		{
			const ScriptInstance &si = scripts[i];
			ReloadFile(si.s);

			if (si.s)
			{
				si.s->CallOnAddEditorProperty(si.e);
				si.s->RemovedUnusedProperties();
			}
		}

		Log::Print(LogLevel::LEVEL_INFO, "Done!\n");
	}

	void ScriptManager::ReloadScripts()
	{
		for (size_t i = 0; i < usedScripts; i++)
		{
			const ScriptInstance& si = scripts[i];

			ReloadFile(si.s);

			if (si.s)
				si.s->ReloadProperties();
		}
	}

	void ScriptManager::ReloadProperties()
	{
		for (size_t i = 0; i < usedScripts; i++)
		{
			const ScriptInstance& si = scripts[i];

			if (si.s)
				si.s->ReloadProperties();
		}
	}

	void ScriptManager::PartialDispose()
	{
		for (size_t i = 0; i < usedScripts; i++)
		{
			const ScriptInstance& si = scripts[i];

			if (si.s)
			{
				si.s->Dispose();
				delete si.s;
			}
		}
		scripts.clear();
	}

	void ScriptManager::Dispose()
	{
		PartialDispose();

		lua_close(L);

		Log::Print(LogLevel::LEVEL_INFO, "Disposing Script manager\n");
	}

	Script *ScriptManager::AddScript(Entity e, const std::string &fileName)
	{
		// Only one script per entity
		if (HasScript(e))
			return scripts[map.at(e.id)].s;

		Script *s = LoadScript(fileName);

		// If the script fails to load, then just return nullptr and continue as if AddScript wasn't called so it doesn't crash
		if (!s)
			return nullptr;

		ScriptInstance si = {};
		si.e = e;
		si.s = s;

		InsertScriptInstance(si);

		Engine::Log::Print(Engine::LogLevel::LEVEL_INFO, "Added script %s to entity %d\n", fileName.c_str(), e.id);

		return s;
	}

	void ScriptManager::DuplicateScript(Entity e, Entity newE)
	{
		if (HasScript(e) == false)
			return;

		const Script *s = GetScript(e);

		Script *newS = LoadScript(s->GetPath());
		const std::vector<ScriptProperty> &props = s->GetProperties();
		for (size_t i = 0; i < props.size(); i++)
		{
			ScriptProperty sp;
			sp.e = newE;
			sp.name = props[i].name;
			newS->properties.push_back(sp);
		}

		ScriptInstance si;
		si.e = newE;
		si.s = newS;

		InsertScriptInstance(si);
	}

	void ScriptManager::LoadFromPrefab(Serializer &s, Entity e)
	{
		std::string path;
		s.Read(path);

		ScriptInstance si = {};

		si.e = e;
		si.s = LoadScript(path);

		if (!si.s)
			return;

		si.s->Deserialize(s);

		InsertScriptInstance(si);

		Engine::Log::Print(Engine::LogLevel::LEVEL_INFO, "Added script %s to entity %d\n", path.c_str(), e.id);
	}

	void ScriptManager::InsertScriptInstance(const ScriptInstance &si)
	{
		if (usedScripts < scripts.size())
		{
			scripts[usedScripts] = si;
			map[si.e.id] = usedScripts;
		}
		else
		{
			scripts.push_back(si);
			map[si.e.id] = (unsigned int)scripts.size() - 1;
		}

		usedScripts++;

		// If there is any disabled entity then we need to swap the new one, which was inserted at the end, with the first disabled entity
		if (disabledScripts > 0)
		{
			// Get the indices of the entity to disable and the last entity
			unsigned int newEntityIndex = usedScripts - 1;
			unsigned int firstDisabledEntityIndex = usedScripts - disabledScripts - 1;					// Get the first entity disabled

			ScriptInstance si1 = scripts[newEntityIndex];
			ScriptInstance si2 = scripts[firstDisabledEntityIndex];

			// Now swap the entities
			scripts[newEntityIndex] = si2;
			scripts[firstDisabledEntityIndex] = si1;

			// Swap the indices
			map[si.e.id] = firstDisabledEntityIndex;
			map[si2.e.id] = newEntityIndex;
		}
	}

	void ScriptManager::RemoveScript(Entity e)
	{
		if (HasScript(e))
		{
			// To remove an entity we need to swap it with the last one, but, because there could be disabled entities at the end
			// we need to first swap the entity to remove with the last ENABLED entity and then if there are any disabled entities, swap the entity to remove again,
			// but this time with the last disabled entity.
			unsigned int entityToRemoveIndex = map.at(e.id);
			unsigned int lastEnabledEntityIndex = usedScripts - disabledScripts - 1;

			ScriptInstance entityToRemoveSi = scripts[entityToRemoveIndex];
			ScriptInstance lastEnabledEntitySi = scripts[lastEnabledEntityIndex];

			// Swap the entity to remove with the last enabled entity
			scripts[lastEnabledEntityIndex] = entityToRemoveSi;
			scripts[entityToRemoveIndex] = lastEnabledEntitySi;

			// Now change the index of the last enabled entity, which is now in the spot of the entity to remove, to the entity to remove index
			map[lastEnabledEntitySi.e.id] = entityToRemoveIndex;
			map.erase(e.id);

			// If there any disabled entities then swap the entity to remove, which is is the spot of the last enabled entity, with the last disabled entity
			if (disabledScripts > 0)
			{
				entityToRemoveIndex = lastEnabledEntityIndex;			// The entity to remove is now in the spot of the last enabled entity
				unsigned int lastDisabledEntityIndex = usedScripts - 1;

				ScriptInstance lastDisabledEntitySi = scripts[lastDisabledEntityIndex];

				scripts[lastDisabledEntityIndex] = entityToRemoveSi;
				scripts[entityToRemoveIndex] = lastDisabledEntitySi;

				map[lastDisabledEntitySi.e.id] = entityToRemoveIndex;
			}

			if (entityToRemoveSi.s != nullptr)
			{
				entityToRemoveSi.s->Dispose();
				delete entityToRemoveSi.s;
			}
			usedScripts--;
		}
	}

	void ScriptManager::SetScriptEnabled(Entity e, bool enable)
	{
		if (HasScript(e))
		{
			if (enable)
			{
				// If we only have one disabled, then there's no need to swap
				if (disabledScripts == 1)
				{
					disabledScripts--;
					return;
				}
				else
				{
					// To enable when there's more than 1 entity disabled just swap the disabled entity that is going to be enabled with the first disabled entity
					unsigned int entityIndex = map.at(e.id);
					unsigned int firstDisabledEntityIndex = usedScripts - disabledScripts;		// Don't subtract -1 because we want the first disabled entity, otherwise we would get the last enabled entity. Eg 6 used, 2 disabled, 6-2=4 the first disabled entity is at index 4 and the second at 5

					ScriptInstance si1 = scripts[entityIndex];
					ScriptInstance si2 = scripts[firstDisabledEntityIndex];

					scripts[entityIndex] = si2;
					scripts[firstDisabledEntityIndex] = si1;

					map[e.id] = firstDisabledEntityIndex;
					map[si2.e.id] = entityIndex;

					disabledScripts--;
				}
			}
			else
			{
				// Get the indices of the entity to disable and the last entity
				unsigned int entityIndex = map.at(e.id);
				unsigned int firstDisabledEntityIndex = usedScripts - disabledScripts - 1;		// Get the first entity disabled or the last entity if none are disabled

				ScriptInstance si1 = scripts[entityIndex];
				ScriptInstance si2 = scripts[firstDisabledEntityIndex];

				// Now swap the entities
				scripts[entityIndex] = si2;
				scripts[firstDisabledEntityIndex] = si1;

				// Swap the indices
				map[e.id] = firstDisabledEntityIndex;
				map[si2.e.id] = entityIndex;

				disabledScripts++;
			}		
		}
	}

	void ScriptManager::ExecuteFile(const std::string &fileName)
	{
		std::ifstream f = game->GetFileManager()->OpenForReading(fileName, std::ios::in | std::ios::ate);

		if (f.fail())
		{
			Log::Print(LogLevel::LEVEL_ERROR, "%s\n", strerror(errno));
			return;
		}

		if (!f.is_open())
		{
			Log::Print(LogLevel::LEVEL_ERROR, "Failed to open script file: %s\n", fileName.c_str());
			return;
		}

		char *scriptString = game->GetFileManager()->ReadEntireFile(f, false);

		if (!scriptString)
		{
			Log::Print(LogLevel::LEVEL_ERROR, "Failed to read script string\n");
			return;
		}

		//Log::Print(LogLevel::LEVEL_INFO, "%s\n", scriptString);

		if (luaL_dostring(L, scriptString) != 0)
		{
			Log::Print(LogLevel::LEVEL_INFO, "Error in script: %s\n", fileName.c_str());
			Log::Print(LogLevel::LEVEL_INFO, "%s\n", lua_tostring(L, -1));
		}

		delete[] scriptString;

		/*if (luaL_dofile(L, fileName.c_str()) != 0)
		{
			std::cout << "Error in script: " << fileName << "\n";
			std::cout << lua_tostring(L, -1) << "\n";
		}*/
	}

	void ScriptManager::ExecuteString(const char *str)
	{
		if (!str)
		{
			Log::Print(LogLevel::LEVEL_ERROR, "ERROR -> Script string is null\n");
			return;
		}

		if (luaL_dostring(L, str) != 0)
		{
			Log::Print(LogLevel::LEVEL_INFO, "Error in script string: %s\n", str);
			Log::Print(LogLevel::LEVEL_INFO, "%s\n", lua_tostring(L, -1));
		}
	}
	
	void ScriptManager::CallFunction(const std::string &functionName)
	{
		luabridge::LuaRef function = luabridge::getGlobal(L, functionName.c_str());
		try
		{
			function();
		}
		catch (luabridge::LuaException const& e)
		{
			std::cout << "LuaException: " << e.what() << "\n";
		}
	}

	bool ScriptManager::HasScript(Entity e) const
	{
		return map.find(e.id) != map.end();
	}

	Script *ScriptManager::GetScript(Entity e) const
	{
		return scripts[map.at(e.id)].s;
	}

	luabridge::LuaRef ScriptManager::GetScriptEnvironment(Entity e) const
	{
		return GetScript(e)->GetEnvironment();
	}

	void ScriptManager::Serialize(Serializer &s, bool playMode) const
	{
		// Store the map, otherwise we have problems with play/stop when we enable/disable entities
		if (playMode)
		{
			s.Write((unsigned int)map.size());
			for (auto it = map.begin(); it != map.end(); it++)
			{
				s.Write(it->first);
				s.Write(it->second);
			}
		}

		s.Write(usedScripts);
		s.Write(disabledScripts);
		for (unsigned int i = 0; i < usedScripts; i++)
		{
			const ScriptInstance &si = scripts[i];
			s.Write(si.e.id);
			si.s->Serialize(s);		
		}	
	}

	void ScriptManager::Deserialize(Serializer &s, bool playMode)
	{
		Log::Print(LogLevel::LEVEL_INFO, "Deserializing script manager\n");

		if (!playMode)
		{
			s.Read(usedScripts);
			s.Read(disabledScripts);
			scripts.resize(usedScripts);

			for (unsigned int i = 0; i < usedScripts; i++)
			{
				ScriptInstance si;
				s.Read(si.e.id);

				std::string path;
				s.Read(path);

				si.s = LoadScript(path);

				// Set these two before CallOnAddEditorProperty otherwise the entity won't be in the map when it gets called
				scripts[i] = si;
				map[si.e.id] = i;

				if (si.s)
				{
					si.s->Deserialize(s);
#ifdef EDITOR
					si.s->CallOnAddEditorProperty(si.e);
					si.s->RemovedUnusedProperties();
#endif
				}
				else
				{
					Log::Print(LogLevel::LEVEL_WARNING, "Removed missing script for entity %u : %s\n", si.e.id, path.c_str());
				}
			}


			// Loop again through all the script, but now to remove the scripts from the entities which have missing scripts
			for (unsigned int i = 0; i < usedScripts; i++)
			{
				if (scripts[i].s == nullptr)
					RemoveScript(scripts[i].e);
			}
		}
		else
		{
			// Read the map to prevent bugs when entities are enabled/disabled with play/stop
			unsigned int mapSize = 0;
			s.Read(mapSize);

			unsigned int eid, idx;
			for (unsigned int i = 0; i < mapSize; i++)
			{
				s.Read(eid);
				s.Read(idx);
				//map[eid] = idx;
			}

			s.Read(usedScripts);
			s.Read(disabledScripts);

			for (unsigned int i = 0; i < usedScripts; i++)
			{
				s.Read(eid);
				unsigned int idx = map[eid];

				ScriptInstance &si = scripts[idx];
				si.e.id = eid;
				std::string path;
				s.Read(path);

				map[eid] = i;

				if (si.s)
				{
					si.s->Deserialize(s);
#ifdef EDITOR
					si.s->CallOnAddEditorProperty(si.e);
					si.s->RemovedUnusedProperties();
#endif
				}
			}	
		}
	}

	std::unordered_map<std::string, luabridge::LuaRef> ScriptManager::GetKeyValueMap(const luabridge::LuaRef &table)
	{
		std::unordered_map<std::string, luabridge::LuaRef> result;

		if (table.isNil())
			return result;

		luabridge::push(L, table);					// push table

		lua_pushnil(L);								// push nil, so lua_next removes it from stack and puts (k, v) on stack
		while (lua_next(L, -2) != 0)				// -2, because we have table at -1
		{
			if (lua_isstring(L, -2))				 // only store stuff with string keys
			{
				std::string s = lua_tostring(L, -2);
				result.emplace(s, luabridge::LuaRef::fromStack(L, -1));
			}
			lua_pop(L, 1);						// remove value, keep key for lua_next
		}

		lua_pop(L, 1);				// pop table

		return result;
	}

	void ScriptManager::ReloadFile(Script *script)
	{
		if (!script)
			return;

		script->Dispose();

		std::string tableName = script->GetPath();
		size_t lastBar = tableName.find_last_of('/');

		tableName.erase(0, lastBar + 1);		// +1 to remove the /

		// Remove the extension and the dot
		tableName.pop_back();
		tableName.pop_back();
		tableName.pop_back();
		tableName.pop_back();

		std::string fileStr;

		std::ifstream file(script->GetPath());
		if (file.is_open())
		{
			std::stringstream buffer;
			buffer << file.rdbuf();
			fileStr = buffer.str();

			size_t tableNamePos = fileStr.find(tableName);
			while (tableNamePos != std::string::npos)
			{
				fileStr.replace(tableNamePos, tableName.length(), script->GetName());
				tableNamePos = fileStr.find(tableName, tableNamePos + tableName.length());	// Advance tableName.length to the offset so when we use find again we don't get the same position	
			}

			file.close();
		}
		else
		{
			std::cout << "Failed to open script file to change: " << script->GetPath() << '\n';
			return;
		}

		if (luaL_dostring(L, fileStr.c_str()) == 0)
		{
			luabridge::LuaRef table = luabridge::getGlobal(L, script->GetName().c_str());

			if (table.isTable())
			{
				ReadTable(script, table);
			}
		}
		else
		{
			std::cout << "Error in script: " << script->GetPath() << "\n";
			std::cout << lua_tostring(L, -1) << "\n";
		}
	}

	void ScriptManager::ReadTable(Script *script, const luabridge::LuaRef &table)
	{
		if (table["onAddEditorProperty"].isFunction())
		{
			script->SetFunction(OnAddEditorProperty, new luabridge::LuaRef(table["onAddEditorProperty"]));
		}
		if (table["onInit"].isFunction())
		{
			script->SetFunction(OnInit, new luabridge::LuaRef(table["onInit"]));
		}
		if (table["onUpdate"].isFunction())
		{
			script->SetFunction(OnUpdate, new luabridge::LuaRef(table["onUpdate"]));
		}
		if (table["onEvent"].isFunction())
		{
			script->SetFunction(OnEvent, new luabridge::LuaRef(table["onEvent"]));
		}
		if (table["onRender"].isFunction())
		{
			script->SetFunction(OnRender, new luabridge::LuaRef(table["onRender"]));
		}
		if (table["onDamage"].isFunction())
		{
			script->SetFunction(OnDamage, new luabridge::LuaRef(table["onDamage"]));
		}
		if (table["onTriggerEnter"].isFunction())
		{
			script->SetFunction(OnTriggerEnter, new luabridge::LuaRef(table["onTriggerEnter"]));
		}
		if (table["onTriggerStay"].isFunction())
		{
			script->SetFunction(OnTriggerStay, new luabridge::LuaRef(table["onTriggerStay"]));
		}
		if (table["onTriggerExit"].isFunction())
		{
			script->SetFunction(OnTriggerExit, new luabridge::LuaRef(table["onTriggerExit"]));
		}
		if (table["onResize"].isFunction())
		{
			script->SetFunction(OnResize, new luabridge::LuaRef(table["onResize"]));
		}
		if (table["onTargetSeen"].isFunction())
		{
			script->SetFunction(OnTargetSeen, new luabridge::LuaRef(table["onTargetSeen"]));
		}
		if (table["onTargetInRange"].isFunction())
		{
			script->SetFunction(OnTargetInRange, new luabridge::LuaRef(table["onTargetInRange"]));
		}
		if (table["onButtonPressed"].isFunction())
		{
			script->SetFunction(OnButtonPressed, new luabridge::LuaRef(table["onButtonPressed"]));
		}
	}

	Script *ScriptManager::LoadScript(const std::string &fileName)
	{
		// Get the file name from the path
		std::string tableName = fileName;
		size_t lastBar = tableName.find_last_of('/');

		tableName.erase(0, lastBar + 1);		// +1 to remove the /

												// Remove the extension and the dot
		tableName.pop_back();
		tableName.pop_back();
		tableName.pop_back();
		tableName.pop_back();

		int occurrences = 0;
		std::string fileStr;
		std::string newTableName;

		for (size_t i = 0; i < usedScripts; i++)
		{
			const ScriptInstance &si = scripts[i];

			if (si.s && si.s->GetName() == tableName)		// The script has already been loaded
				occurrences++;
		}
		if (occurrences > 0)
		{
			std::ifstream file = game->GetFileManager()->OpenForReading(fileName);

			if (!file.is_open())
			{
				Log::Print(LogLevel::LEVEL_ERROR, "Failed to open script file: %s\n", fileName.c_str());
				return nullptr;
			}

			std::stringstream buffer;
			buffer << file.rdbuf();
			fileStr = buffer.str();

			size_t tableNamePos = fileStr.find(tableName);
			while (tableNamePos != std::string::npos)
			{
				newTableName = tableName + std::to_string(usedScripts);				// Append the script count instead of occurrences otherwise we will have errors

				fileStr.replace(tableNamePos, tableName.length(), newTableName);
				tableNamePos = fileStr.find(tableName, tableNamePos + tableName.length());	// Advance tableName.length to the offset so when we use find again we don't get the same position	
			}

			/*std::ofstream outF("Data/test.lua");
			outF << fileStr;
			outF.close();*/
			file.close();
		}

		if (occurrences > 0)
		{
			if (luaL_dostring(L, fileStr.c_str()) == 0)
			{
				luabridge::LuaRef table = luabridge::getGlobal(L, newTableName.c_str());

				if (table.isTable())
				{
					Script *script = new Script(L, newTableName, fileName, table);		// Use the old table name so we can keep track of the number of occurrences
					ReadTable(script, table);
					return script;
				}
			}
			else
			{
				Log::Print(LogLevel::LEVEL_ERROR, "Error in script: %s\n", fileName.c_str());
				Log::Print(LogLevel::LEVEL_ERROR, "%s\n", lua_tostring(L, -1));
				return nullptr;
			}
		}
		else
		{		
			Engine::Log::Print(Engine::LogLevel::LEVEL_INFO, "Loading script\n");

			std::ifstream f = game->GetFileManager()->OpenForReading(fileName, std::ios::ate);

			if (!f.is_open())
			{
				Log::Print(LogLevel::LEVEL_ERROR, "Failed to open script file: %s\n", fileName.c_str());
				return nullptr;
			}

			char *scriptString = game->GetFileManager()->ReadEntireFile(f, false);

			if (!scriptString)
			{
				Log::Print(LogLevel::LEVEL_ERROR, "Failed to read script string\n");
				return nullptr;
			}

			if (luaL_dostring(L, scriptString) == 0)
			{
				Engine::Log::Print(Engine::LogLevel::LEVEL_INFO, "Loaded script\n");
				luabridge::LuaRef table = luabridge::getGlobal(L, tableName.c_str());

				if (table.isTable())
				{
					Engine::Log::Print(Engine::LogLevel::LEVEL_INFO, "Reading table\n");
					Script *script = new Script(L, tableName, fileName, table);
					ReadTable(script, table);
					return script;
				}
			}
			else
			{
				Log::Print(LogLevel::LEVEL_ERROR, "Error in script: %s\n", fileName.c_str());
				Log::Print(LogLevel::LEVEL_ERROR, "%s\n", lua_tostring(L, -1));
				return nullptr;
			}
		}

		return nullptr;
	}
}
