#include "SoundSource.h"

namespace Engine
{
	SoundSource::SoundSource()
	{
		//sound = nullptr;
		//channel = nullptr;
		isStream = false;
		wantsPlay = false;
		wantsStop = false;
		isPlaying = false;
		volume = 1.0f;
		pitch = 1.0f;
		//position = { 0.0f, 0.0f, 0.0f };
		min3DDistance = 1.0f;
		max3DDistance = 100.0f;
		is3D = false;
		isLooping = false;
		playOnStart = false;
	}

	/*void SoundSource::SetSound(FMOD::Sound *sound)
	{
		if (!sound)
			return;

		this->sound = sound;

		if (is3D)
		{
			sound->setMode(FMOD_3D);
			sound->setMode(FMOD_3D_LINEARROLLOFF);
			//sound->set3DMinMaxDistance(min3DDistance, max3DDistance);
		}
		if (isLooping)
		{
			sound->setMode(FMOD_LOOP_NORMAL);
		}
	}*/

	void SoundSource::SetMin3DDistance(float distance)
	{
		min3DDistance = distance;

		//if (sound)
			//sound->set3DMinMaxDistance(min3DDistance, max3DDistance);
	}

	void SoundSource::SetMax3DDistance(float distance)
	{
		max3DDistance = distance;

		//if (sound)
			//sound->set3DMinMaxDistance(min3DDistance, max3DDistance);
	}

	void SoundSource::Enable3D(bool enable)
	{
		/*if (sound)
		{
			if (enable)
			{
				is3D = true;
				sound->setMode(FMOD_3D);
				sound->setMode(FMOD_3D_LINEARROLLOFF);
				//sound->set3DMinMaxDistance(min3DDistance, max3DDistance);
			}
			else
			{
				is3D = false;
				sound->setMode(FMOD_2D);
			}
		}*/
	}

	void SoundSource::EnableLoop(bool enable)
	{
		/*if (sound)
		{
			if (enable)
			{
				isLooping = true;
				sound->setMode(FMOD_LOOP_NORMAL);
			}
			else
			{
				isLooping = false;
				sound->setMode(FMOD_LOOP_OFF);
			}
		}*/
	}

	void SoundSource::Serialize(Serializer &s)
	{
		s.Write(path);
		s.Write(volume);
		s.Write(pitch);
		s.Write(is3D);
		s.Write(isLooping);
		s.Write(playOnStart);
		s.Write(min3DDistance);
		s.Write(max3DDistance);
		s.Write(isStream);
	}

	void SoundSource::Deserialize(Serializer &s)
	{
		s.Read(path);
		s.Read(volume);
		s.Read(pitch);
		s.Read(is3D);
		s.Read(isLooping);
		s.Read(playOnStart);
		s.Read(min3DDistance);
		s.Read(max3DDistance);
		s.Read(isStream);
	}
}
