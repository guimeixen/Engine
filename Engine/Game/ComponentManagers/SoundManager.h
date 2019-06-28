#pragma once

//#include "include/FMOD/fmod_common.h"

#include "Sound/SoundSource.h"
#include "Game/EntityManager.h"

#include <vector>
#include <string>
#include <unordered_map>

namespace Engine
{
	class Game;
	class TransformManager;

	struct SoundInfo
	{
		//FMOD::Sound *sound;
		unsigned int id;
	};

	struct SoundSourceInstance
	{
		Entity e;
		SoundSource *ss;
	};

	class SoundManager
	{
	public:
		SoundManager();

		bool Init(Game *game, TransformManager *transformManager);
		void Update(const glm::vec3 &listenerPos);
		void Play();
		SoundSource *AddSoundSource(Entity e);
		void DuplicateSoundSource(Entity e, Entity newE);
		void SetSoundSourceEnabled(Entity e, bool enable);
		void LoadSound(SoundSource *soundSource, const std::string &path, bool stream);
		void ReloadSound(SoundSource *soundSource);
		void RemoveSoundSource(Entity e);
		void Dispose();

		bool HasSoundSource(Entity e) const;
		SoundSource *GetSoundSource(Entity e) const;

		void Serialize(Serializer &s) const;
		void Deserialize(Serializer &s, bool reload = false);

		// Script functions
		//FMOD::Sound *LoadSoundEffect(const std::string &path);		// Path is relative to the project folder
		//FMOD::Sound *LoadMusic(const std::string &path);

	private:
		void InsertSoundSourceInstance(const SoundSourceInstance &ssi);

	private:
		Game *game;
		TransformManager *transformManager;
		//FMOD::System *fmodSystem;
		bool isInit;
		std::vector<SoundInfo> sounds;
		std::vector<SoundSourceInstance> soundSources;
		std::unordered_map<unsigned int, unsigned int> map;
		unsigned int usedSoundSources;
		unsigned int disabledSoundSources;
	};
}
