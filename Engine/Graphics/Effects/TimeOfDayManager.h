#pragma once

#include "Graphics\Mesh.h"

namespace Engine
{
	class Texture;
	struct MaterialInstance;
	class Game;
	class Renderer;

	struct TimeInfo
	{
		float time;
		glm::vec3 dirLightColor;
		float intensity;
		glm::vec3 dirLightDirection;
		float ambient;
		glm::vec3 fogInscatteringColor;
		float heightFogDensity;
		glm::vec3 lightInscaterringColor;
		glm::vec3 skyColor;
		float lightShaftsIntensity;
	};

	class TimeOfDayManager
	{
	public:
		TimeOfDayManager();
		~TimeOfDayManager();

		void Init(Game *game);
		void Update(float dt);
		void Render(Renderer *renderer);
		void Dispose();

		void SetCurrentTime(float time);
		void SetAzimuthOffset(float offset);
		void SetCurrentDay(int day);
		void SetCurrentMonth(int month);

		float GetCurrentTime() const { return worldTime; }
		float GetAzimuthOffset() const { return azimuthOffset; }
		float GetSunriseTime() const { return sunriseTime; }
		float GetSunsetTime() const { return sunsetTime; }
		int GetCurrentMonth() const { return (int)month;}
		int GetCurrentDay() const { return (int)day; }

		const TimeInfo &GetCurrentTimeInfo() const { return currentTimeInfo; }

	private:
		void BlendTimeInfo(const TimeInfo &t1, const TimeInfo &t2);

		float DaysInMonth(int m, int y);
		void DayOfYear();
		void CalculateDayValues();
		void CalculateSunriseSunset();

	private:
		Game *game;
		Mesh skydomeMesh;
		MaterialInstance *skydomeMat;
		Texture *transmittanceTexture;
		Texture *inscatterTexture;

		static const unsigned int MAX_TIME_INFOS = 7;
		TimeInfo timeInfos[MAX_TIME_INFOS];
		TimeInfo currentTimeInfo;
		float a = 0.0f;
		unsigned int curIndex = 0;
		unsigned int nextIndex = 0;

		float worldTime = 13.0f;

		int year = 2019;
		float day = 25.0f;
		float month = 1.0f;
		float hour = 13.0f;
		float offset = 1.0f;
		float minute = 51.0f;
		float lat = 42.545f;
		float longi = -8.428f;
		float dayOfYear = 0.0f;
		float azimuth = 0.0f;
		float altitude = 0.0f;
		float azimuthOffset = 40.5f;

		float sunriseTime = 0.0f;
		float sunsetTime = 0.0f;
	};

}
