#pragma once

#include "Program/Serializer.h"

//#include "include/FMOD/fmod.hpp"

#include <string>

namespace Engine
{
	class SoundSource
	{
	private:
		friend class SoundManager;
	public:
		SoundSource();

		//FMOD::Sound *GetSoundHandle() const { return sound; }

		void Play() { wantsPlay = true; wantsStop = false; }
		void Stop() { wantsPlay = false; wantsStop = true; }

		//void SetSound(FMOD::Sound *sound);
		void SetVolume(float v) { volume = v;  if (volume <= 0.0f) volume = 0.01f; }
		void SetPitch(float p) { pitch = p; if (pitch <= 0.0f) pitch = 0.01f; }
		void SetPosition(const glm::vec3 &pos) { /*position.x = pos.x; position.y = pos.y; position.z = pos.z;*/ }
		void SetMin3DDistance(float distance);
		void SetMax3DDistance(float distance);
		void Enable3D(bool enable);
		void EnableLoop(bool enable);
		void EnablePlayOnStart(bool enable) { playOnStart = enable; }

		float GetVolume() const { return volume; }
		float GetPitch() const { return pitch; }
		float GetMin3DDistance() const { return min3DDistance; }
		float GetMax3DDistance() const { return max3DDistance; }
		bool IsPlaying() const { return isPlaying; }
		bool Is3D() const { return is3D; }
		bool IsLooping() const { return isLooping; }
		bool IsPlayingOnStart() const { return playOnStart; }
		bool IsStream() const { return isStream; }
		const std::string &GetPath() const { return path; }		

		void Serialize(Serializer &s);
		void Deserialize(Serializer &s);

	private:
		//FMOD::Sound *sound;
		//FMOD::Channel *channel;
		std::string path;
		bool isStream;
		bool wantsPlay;
		bool wantsStop;
		bool isPlaying;
		float volume;
		float pitch;
		//FMOD_VECTOR position;
		float min3DDistance;
		float max3DDistance;
		bool is3D;
		bool isLooping;
		bool playOnStart;
	};
}
