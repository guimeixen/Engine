#include "SoundManager.h"

#include "Game\Game.h"

#include "Program\Utils.h"
#include "Program\StringID.h"

#include "include\FMOD\fmod_errors.h"

#include <iostream>

namespace Engine
{
	SoundManager::SoundManager()
	{
		isInit = false;
	}

	SoundManager::~SoundManager()
	{
	}

	bool SoundManager::Init(Game *game, TransformManager *transformManager)
	{
		if (isInit)
			return false;

		this->game = game;
		this->transformManager = transformManager;

		if (FMOD::System_Create(&fmodSystem) != FMOD_OK)
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


		fmodSystem->set3DSettings(1.0f, 1.0f, 1.0f);

		isInit = true;

		return true;
	}

	void SoundManager::Update(const glm::vec3 &listenerPos)
	{
		if (!isInit)
			return;

		for (size_t i = 0; i < usedSoundSources; i++)
		{
			const SoundSourceInstance &ssi = soundSources[i];
			SoundSource *ss = ssi.ss;

			ss->SetPosition(transformManager->GetLocalToWorld(ssi.e)[3]);

			if (ss->wantsPlay && !ss->isPlaying && !ss->wantsStop)
			{
				fmodSystem->playSound(ss->sound, nullptr, false, &ss->channel);
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

		fmodSystem->update();
	}

	void SoundManager::Play()
	{
		for (size_t i = 0; i < usedSoundSources; i++)
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

		if (usedSoundSources < soundSources.size())
		{
			soundSources[usedSoundSources] = ssi;
			map[e.id] = usedSoundSources;
		}
		else
		{
			soundSources.push_back(ssi);
			map[e.id] = (unsigned int)soundSources.size() - 1;
		}

		usedSoundSources++;

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
		newSS->position = ss->position;
		
		LoadSound(newSS, ss->path, ss->isStream);			// Load sound sets the path and isStream variables of sound source

		SoundSourceInstance ssi;
		ssi.e = newE;
		ssi.ss = newSS;

		if (usedSoundSources < soundSources.size())
		{
			soundSources[usedSoundSources] = ssi;
			map[newE.id] = usedSoundSources;
		}
		else
		{
			soundSources.push_back(ssi);
			map[newE.id] = (unsigned int)soundSources.size() - 1;
		}

		usedSoundSources++;
	}

	void SoundManager::LoadSound(SoundSource *soundSource, const std::string &path, bool stream)
	{
		if (!isInit)
			return;

		// Check if we already have the sound stored
		unsigned int id = SID(path);

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
		}
	}

	void SoundManager::ReloadSound(SoundSource *soundSource)
	{
		LoadSound(soundSource, soundSource->path, soundSource->isStream);
	}

	void SoundManager::RemoveSoundSource(Entity e)
	{
		if (HasSoundSource(e))
		{
			unsigned int index = map.at(e.id);

			SoundSourceInstance temp = soundSources[index];
			SoundSourceInstance last = soundSources[soundSources.size() - 1];
			soundSources[soundSources.size() - 1] = temp;
			soundSources[index] = last;

			map[last.e.id] = index;
			map.erase(e.id);

			delete temp.ss;
			usedSoundSources--;
		}
	}

	void SoundManager::Dispose()
	{
		if (isInit)
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
		}
	}

	bool SoundManager::HasSoundSource(Entity e) const
	{
		return map.find(e.id) != map.end();
	}

	SoundSource *SoundManager::GetSoundSource(Entity e) const
	{
		return soundSources[map.at(e.id)].ss;
	}

	void SoundManager::Serialize(Serializer &s) const
	{
		s.Write(usedSoundSources);
		for (unsigned int i = 0; i < usedSoundSources; i++)
		{
			const SoundSourceInstance &ssi = soundSources[i];
			s.Write(ssi.e.id);
			ssi.ss->Serialize(s);
		}
	}

	void SoundManager::Deserialize(Serializer &s, bool reload)
	{
		if (!reload)
		{
			s.Read(usedSoundSources);
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
			s.Read(usedSoundSources);
			for (unsigned int i = 0; i < usedSoundSources; i++)
			{
				SoundSourceInstance &ssi = soundSources[i];
				s.Read(ssi.e.id);
				ssi.ss->Deserialize(s);
				ReloadSound(ssi.ss);
			}
		}
	}

	FMOD::Sound *SoundManager::LoadSoundEffect(const std::string &path)
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
	}

	FMOD::Sound *SoundManager::LoadMusic(const std::string &path)
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
	}
}
