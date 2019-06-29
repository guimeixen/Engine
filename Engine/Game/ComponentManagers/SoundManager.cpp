#include "SoundManager.h"

#include "Game/Game.h"

#include "Program/Utils.h"
#include "Program/StringID.h"
#include "Program/Log.h"

//#include "include/FMOD/fmod_errors.h"

#include <iostream>

namespace Engine
{
	SoundManager::SoundManager()
	{
		isInit = false;
		usedSoundSources = 0;
		disabledSoundSources = 0;
	}

	bool SoundManager::Init(Game *game, TransformManager *transformManager)
	{
		if (isInit)
			return false;

		this->game = game;
		this->transformManager = transformManager;

		/*if (FMOD::System_Create(&fmodSystem) != FMOD_OK)
		{
			std::cout << "Error -> Failed to create FMOD System!\n";
			return false;
		}

		unsigned int version = 0;
		fmodSystem->getVersion(&version);

		if (version != FMOD_VERSION)
		{
			std::cout << "Error -> Wrong FMOD version! This program needs version " << FMOD_VERSION << '\n';
			return false;
		}

		int driverCount = 0;
		fmodSystem->getNumDrivers(&driverCount);
		if (driverCount == 0)
		{
			std::cout << "Error -> No sound cards found!\n";
			return false;
		}

		// Init FMOD with 64 virtual channels
		FMOD_RESULT res = fmodSystem->init(64, FMOD_INIT_3D_RIGHTHANDED, nullptr);		// Use righthanded
		if (res != FMOD_OK)
		{
			std::cout << "Error -> Failed to intitalize FMOD System: " << res << '\n';
			return false;
		}


		fmodSystem->set3DSettings(1.0f, 1.0f, 1.0f);*/

		isInit = true;

		Log::Print(LogLevel::LEVEL_INFO, "Init Sound manager\n");

		return true;
	}

	void SoundManager::Update(const glm::vec3 &listenerPos)
	{
		if (!isInit)
			return;

		const unsigned int numEnabledSoundSources = usedSoundSources - disabledSoundSources;

		/*for (size_t i = 0; i < numEnabledSoundSources; i++)
		{
			const SoundSourceInstance &ssi = soundSources[i];
			SoundSource *ss = ssi.ss;

			ss->SetPosition(transformManager->GetLocalToWorld(ssi.e)[3]);

			if (ss->wantsPlay && !ss->isPlaying && !ss->wantsStop)
			{
				/fmodSystem->playSound(ss->sound, nullptr, false, &ss->channel);
				ss->wantsPlay = false;
			}

			if (ss->channel)
			{
				ss->channel->isPlaying(&ss->isPlaying);

				if (ss->isPlaying)
				{
					ss->channel->setVolume(ss->volume);
					ss->channel->setPitch(ss->pitch);
					ss->channel->set3DMinMaxDistance(ss->min3DDistance, ss->max3DDistance);
					if (ss->wantsStop)
					{
						ss->channel->stop();
						ss->wantsStop = false;
						ss->isPlaying = false;
					}

					if (ss->Is3D())
					{
						FMOD_VECTOR velocity = { 0.0f, 0.0f, 0.0f };
						ss->channel->set3DAttributes(&ss->position, &velocity);
					}
				}
			}
		}

		FMOD_VECTOR listenerPosition = { listenerPos.x, listenerPos.y, listenerPos.z };
		FMOD_VECTOR forward = { 0.0f, 0.0f, 1.0f };
		FMOD_VECTOR up = { 0.0f, 1.0f, 0.0f };
		FMOD_VECTOR velocity = { 0.0f, 0.0f, 0.0f };
		fmodSystem->set3DListenerAttributes(0, &listenerPosition, &velocity, &forward, &up);

		fmodSystem->update();*/
	}

	void SoundManager::Play()
	{
		const unsigned int numEnabledSoundSources = usedSoundSources - disabledSoundSources;

		for (size_t i = 0; i < numEnabledSoundSources; i++)
		{
			SoundSource *ss = soundSources[i].ss;
			if (ss->playOnStart)
			{
				ss->wantsPlay = true;
				ss->wantsStop = false;
			}
		}
	}

	SoundSource *SoundManager::AddSoundSource(Entity e)
	{
		SoundSource *soundSource = new SoundSource;
		soundSource->volume = 1.0f;
		soundSource->pitch = 1.0f;

		SoundSourceInstance ssi = {};
		ssi.e = e;
		ssi.ss = soundSource;

		InsertSoundSourceInstance(ssi);

		return soundSource;
	}

	void SoundManager::DuplicateSoundSource(Entity e, Entity newE)
	{
		if (HasSoundSource(e) == false)
			return;

		const SoundSource *ss = GetSoundSource(e);

		SoundSource *newSS = new SoundSource;
		newSS->volume = ss->volume;
		newSS->pitch = ss->pitch;
		newSS->is3D = ss->is3D;
		newSS->isLooping = ss->isLooping;
		newSS->playOnStart = ss->playOnStart;
		newSS->min3DDistance = ss->min3DDistance;
		newSS->max3DDistance = ss->max3DDistance;
		///newSS->position = ss->position;
		
		LoadSound(newSS, ss->path, ss->isStream);			// Load sound sets the path and isStream variables of sound source

		SoundSourceInstance ssi;
		ssi.e = newE;
		ssi.ss = newSS;

		InsertSoundSourceInstance(ssi);
	}

	void SoundManager::SetSoundSourceEnabled(Entity e, bool enable)
	{
		// TODO: This is not finished. We probably want to play/stop the sound when the entity gets enabled/disabled

		if (HasSoundSource(e))
		{
			if (enable)
			{
				// If we only have one disabled, then there's no need to swap
				if (disabledSoundSources == 1)
				{
					disabledSoundSources--;
					return;
				}
				else
				{
					// To enable when there's more than 1 entity disabled just swap the disabled entity that is going to be enabled with the first disabled entity
					unsigned int entityIndex = map.at(e.id);
					unsigned int firstDisabledEntityIndex = usedSoundSources - disabledSoundSources;		// Don't subtract -1 because we want the first disabled entity, otherwise we would get the last enabled entity. Eg 6 used, 2 disabled, 6-2=4 the first disabled entity is at index 4 and the second at 5

					SoundSourceInstance ssi1 = soundSources[entityIndex];
					SoundSourceInstance ssi2 = soundSources[firstDisabledEntityIndex];

					soundSources[entityIndex] = ssi2;
					soundSources[firstDisabledEntityIndex] = ssi1;

					map[e.id] = firstDisabledEntityIndex;
					map[ssi2.e.id] = entityIndex;

					disabledSoundSources--;
				}
			}
			else
			{
				// Get the indices of the entity to disable and the last entity
				unsigned int entityIndex = map.at(e.id);
				unsigned int firstDisabledEntityIndex = usedSoundSources - disabledSoundSources - 1;		// Get the first entity disabled or the last entity if none are disabled

				SoundSourceInstance ssi1 = soundSources[entityIndex];
				SoundSourceInstance ssi2 = soundSources[firstDisabledEntityIndex];

				// Now swap the entities
				soundSources[entityIndex] = ssi2;
				soundSources[firstDisabledEntityIndex] = ssi1;

				// Swap the indices
				map[e.id] = firstDisabledEntityIndex;
				map[ssi2.e.id] = entityIndex;

				disabledSoundSources++;
			}
		}
	}

	void SoundManager::InsertSoundSourceInstance(const SoundSourceInstance &ssi)
	{
		if (usedSoundSources < soundSources.size())
		{
			soundSources[usedSoundSources] = ssi;
			map[ssi.e.id] = usedSoundSources;
		}
		else
		{
			soundSources.push_back(ssi);
			map[ssi.e.id] = (unsigned int)soundSources.size() - 1;
		}

		usedSoundSources++;

		// If there is any disabled entity then we need to swap the new one, which was inserted at the end, with the first disabled entity
		if (disabledSoundSources > 0)
		{
			// Get the indices of the entity to disable and the last entity
			unsigned int newEntityIndex = usedSoundSources - 1;
			unsigned int firstDisabledEntityIndex = usedSoundSources - disabledSoundSources - 1;		// Get the first entity disabled

			SoundSourceInstance ssi1 = soundSources[newEntityIndex];
			SoundSourceInstance ssi2 = soundSources[firstDisabledEntityIndex];

			// Now swap the entities
			soundSources[newEntityIndex] = ssi2;
			soundSources[firstDisabledEntityIndex] = ssi1;

			// Swap the indices
			map[ssi.e.id] = firstDisabledEntityIndex;
			map[ssi2.e.id] = newEntityIndex;
		}
	}

	void SoundManager::LoadSound(SoundSource *soundSource, const std::string &path, bool stream)
	{
		if (!isInit)
			return;

		// Check if we already have the sound stored
		/*unsigned int id = SID(path);

		for (size_t i = 0; i < sounds.size(); i++)
		{
			if (sounds[i].id == id)
			{
				soundSource->sound = sounds[i].sound;
				soundSource->path = path;
				soundSource->isStream = stream;
				return;
			}
		}

		SoundInfo info = {};

		if (stream)
		{
			FMOD_RESULT res = fmodSystem->createStream(path.c_str(), FMOD_DEFAULT, nullptr, &info.sound);
			if (res != FMOD_OK)
			{
				std::cout << "Error -> Failed to create sound stream: " << path << "\nError code: " << res << '\n';
				return;
			}
		}
		else
		{
			FMOD_RESULT res = fmodSystem->createSound(path.c_str(), FMOD_DEFAULT, nullptr, &info.sound);
			if (res != FMOD_OK)
			{
				std::cout << "Error -> Failed to create sound : " << path << "\nError code: " << res << '\n';
				return;
			}
		}

		if (info.sound)
		{
			info.id = id;
			sounds.push_back(info);
			soundSource->sound = info.sound;
			soundSource->path = path;
			soundSource->isStream = stream;
			if (soundSource->is3D)
			{
				info.sound->setMode(FMOD_3D);
				info.sound->setMode(FMOD_3D_LINEARROLLOFF);
				info.sound->set3DMinMaxDistance(soundSource->min3DDistance, soundSource->max3DDistance);
			}
			if (soundSource->isLooping)
			{
				info.sound->setMode(FMOD_LOOP_NORMAL);
			}
		}*/
	}

	void SoundManager::ReloadSound(SoundSource *soundSource)
	{
		LoadSound(soundSource, soundSource->path, soundSource->isStream);
	}

	void SoundManager::RemoveSoundSource(Entity e)
	{
		if (HasSoundSource(e))
		{
			// To remove an entity we need to swap it with the last one, but, because there could be disabled entities at the end
			// we need to first swap the entity to remove with the last ENABLED entity and then if there are any disabled entities, swap the entity to remove again,
			// but this time with the last disabled entity.
			unsigned int entityToRemoveIndex = map.at(e.id);
			unsigned int lastEnabledEntityIndex = usedSoundSources - disabledSoundSources - 1;

			SoundSourceInstance entityToRemoveSsi = soundSources[entityToRemoveIndex];
			SoundSourceInstance lastEnabledEntitySsi = soundSources[lastEnabledEntityIndex];

			// Swap the entity to remove with the last enabled entity
			soundSources[lastEnabledEntityIndex] = entityToRemoveSsi;
			soundSources[entityToRemoveIndex] = lastEnabledEntitySsi;

			// Now change the index of the last enabled entity, which is now in the spot of the entity to remove, to the entity to remove index
			map[lastEnabledEntitySsi.e.id] = entityToRemoveIndex;
			map.erase(e.id);

			// If there any disabled entities then swap the entity to remove, which is is the spot of the last enabled entity, with the last disabled entity
			if (disabledSoundSources > 0)
			{
				entityToRemoveIndex = lastEnabledEntityIndex;			// The entity to remove is now in the spot of the last enabled entity
				unsigned int lastDisabledEntityIndex = usedSoundSources - 1;

				SoundSourceInstance lastDisabledEntitySsi = soundSources[lastDisabledEntityIndex];

				soundSources[lastDisabledEntityIndex] = entityToRemoveSsi;
				soundSources[entityToRemoveIndex] = lastDisabledEntitySsi;

				map[lastDisabledEntitySsi.e.id] = entityToRemoveIndex;
			}

			delete entityToRemoveSsi.ss;
			usedSoundSources--;
		}
	}

	void SoundManager::Dispose()
	{
		/*if (isInit)
		{
			FMOD_RESULT res;
			for (size_t i = 0; i < sounds.size(); i++)
			{
				res = sounds[i].sound->release();
				if (res != FMOD_OK)
					std::cout << FMOD_ErrorString(res) << '\n';
			}
			sounds.clear();

			for (size_t i = 0; i < usedSoundSources; i++)
			{
				if (soundSources[i].ss)
					delete soundSources[i].ss;
			}
			soundSources.clear();

			fmodSystem->close();
			fmodSystem->release();
			isInit = false;
		}*/

		Log::Print(LogLevel::LEVEL_INFO, "Disposing Sound manager\n");
	}

	bool SoundManager::HasSoundSource(Entity e) const
	{
		return map.find(e.id) != map.end();
	}

	SoundSource *SoundManager::GetSoundSource(Entity e) const
	{
		return soundSources[map.at(e.id)].ss;
	}

	void SoundManager::Serialize(Serializer &s, bool playMode) const
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

		s.Write(usedSoundSources);
		s.Write(disabledSoundSources);
		for (unsigned int i = 0; i < usedSoundSources; i++)
		{
			const SoundSourceInstance &ssi = soundSources[i];
			s.Write(ssi.e.id);
			ssi.ss->Serialize(s);
		}	
	}

	void SoundManager::Deserialize(Serializer &s, bool playMode)
	{
		Log::Print(LogLevel::LEVEL_INFO, "Deserializing sound manager\n");

		if (!playMode)
		{
			s.Read(usedSoundSources);
			s.Read(disabledSoundSources);
			soundSources.resize(usedSoundSources);
			for (unsigned int i = 0; i < usedSoundSources; i++)
			{
				SoundSourceInstance ssi;
				s.Read(ssi.e.id);

				ssi.ss = new SoundSource;
				ssi.ss->Deserialize(s);
				ReloadSound(ssi.ss);

				soundSources[i] = ssi;
				map[ssi.e.id] = i;
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
				map[eid] = idx;
			}

			s.Read(usedSoundSources);
			s.Read(disabledSoundSources);

			for (unsigned int i = 0; i < usedSoundSources; i++)
			{		
				s.Read(eid);
				unsigned int idx = map[eid];

				SoundSourceInstance &ssi = soundSources[idx];
				ssi.e.id = eid;
				ssi.ss->Deserialize(s);
				ReloadSound(ssi.ss);
			}		
		}
	}

	/*FMOD::Sound *SoundManager::LoadSoundEffect(const std::string &path)
	{
		// Check if we already have the sound stored
		//std::string realPath = "Data/Levels/" + game->GetProjectName() + '/' + path;
		std::string realPath = game->GetProjectDir() + path;

		unsigned int id = SID(realPath);

		for (size_t i = 0; i < sounds.size(); i++)
		{
			if (sounds[i].id == id)
			{
				return sounds[i].sound;
			}
		}

		SoundInfo info = {};

		FMOD_RESULT res = fmodSystem->createSound(realPath.c_str(), FMOD_DEFAULT, nullptr, &info.sound);
		if (res != FMOD_OK)
		{
			std::cout << "Error -> Failed to create sound : " << realPath << "\nError code: " << res << '\n';
			return nullptr;
		}

		if (info.sound)
		{
			info.id = id;
			sounds.push_back(info);
		}

		return info.sound;
		return nullptr;
	}*/

	/*FMOD::Sound *SoundManager::LoadMusic(const std::string &path)
	{
		// Check if we already have the sound stored
		//std::string realPath = "Data/Levels/" + game->GetProjectName() + '/' + path;
		std::string realPath = game->GetProjectDir() + path;

		unsigned int id = SID(realPath);

		for (size_t i = 0; i < sounds.size(); i++)
		{
			if (sounds[i].id == id)
			{
				return sounds[i].sound;
			}
		}

		SoundInfo info = {};

		FMOD_RESULT res = fmodSystem->createStream(realPath.c_str(), FMOD_DEFAULT, nullptr, &info.sound);
		if (res != FMOD_OK)
		{
			std::cout << "Error -> Failed to create sound stream : " << realPath << "\nError code: " << res << '\n';
			return nullptr;
		}

		if (info.sound)
		{
			info.id = id;
			sounds.push_back(info);
		}

		return info.sound;
	}*/
}
