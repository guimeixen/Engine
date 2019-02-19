#include "TimeOfDayManager.h"

#include "Game\Game.h"
#include "Graphics\Texture.h"
#include "Graphics\ResourcesLoader.h"
#include "Graphics\MeshDefaults.h"
#include "Graphics\VertexArray.h"
#include "Graphics\Renderer.h"
#include "Graphics\Material.h"

#include "Program\Input.h"

#include "include\glm\gtc\matrix_transform.hpp"

#include <iostream>

namespace Engine
{
	TimeOfDayManager::TimeOfDayManager()
	{
		transmittanceTexture = nullptr;
		inscatterTexture = nullptr;
		skydomeMesh = {};
	}

	TimeOfDayManager::~TimeOfDayManager()
	{
	}

	void TimeOfDayManager::Init(Game *game)
	{
		this->game = game;

		FILE *file = fopen("Data/Resources/Textures/transmittance.raw", "rb");

		if (file == nullptr)
		{
			std::cout << "transmittance file is null";
			return;
		}

		float *transmittanceData = new float[256 * 64 * 3];
		fread(transmittanceData, 1, 256 * 64 * 3 * sizeof(float), file);
		fclose(file);

		file = fopen("Data/Resources/Textures/inscatter.raw", "rb");
		if (file == nullptr)
		{
			std::cout << "inscatter file is null";
			delete[] transmittanceData;
			return;
		}

		float* inscatterData = new float[256 * 128 * 32 * 4];
		fread(inscatterData, 1, 256 * 128 * 32 * 4 * sizeof(float), file);
		fclose(file);

		TextureParams params = {};
		params.wrap = TextureWrap::CLAMP_TO_EDGE;
		params.filter = TextureFilter::LINEAR;
		params.format = TextureFormat::RGB;
		params.internalFormat = TextureInternalFormat::RGB16F;
		params.type = TextureDataType::FLOAT;
		params.useMipmapping = false;
		params.enableCompare = false;

		transmittanceTexture = game->GetRenderer()->CreateTexture2DFromData(256, 64, params, transmittanceData);

		params.format = TextureFormat::RGBA;
		params.internalFormat = TextureInternalFormat::RGBA16F;

		inscatterTexture = game->GetRenderer()->CreateTexture3DFromData(256, 128, 32, params, inscatterData);

		delete[] transmittanceData;
		delete[] inscatterData;

		skydomeMesh = MeshDefaults::CreateSphere(game->GetRenderer());
		skydomeMat = game->GetRenderer()->CreateMaterialInstanceFromBaseMat(game->GetScriptManager(), "Data/Resources/Materials/skydome_mat.lua", skydomeMesh.vao->GetVertexInputDescs());

		if (skydomeMat->textures.size() < 2)
		{
			Dispose();
			return;
		}

		skydomeMat->textures[0] = transmittanceTexture;
		skydomeMat->textures[1] = inscatterTexture;
		game->GetRenderer()->UpdateMaterialInstance(skydomeMat);
		

		// Sunrise
		TimeInfo t7 = {};
		t7.time = 7.2f;
		t7.dirLightColor = glm::vec3(47.0f / 255.0f, 82.0f / 255.0f, 101.0f / 255.0f);
		t7.intensity = 0.04f;
		t7.ambient = 0.013f;
		t7.heightFogDensity = 0.0001f;
		t7.fogInscatteringColor = glm::vec3(8.0f / 255.0f, 16.0f / 255.0f, 61.0f / 255.0f);
		t7.lightInscaterringColor = glm::vec3(8.0f / 255.0f, 16.0f / 255.0f, 61.0f / 255.0f);
		t7.skyColor = glm::vec3(0.0f, 0.02f, 0.07f);
		t7.lightShaftsIntensity = 0.0f;

		TimeInfo t1 = {};
		t1.time = 8.3f;
		t1.dirLightColor = glm::vec3(255.0f / 255.0f, 80.0f / 255.0f, 0.0f);
		t1.intensity = 1.0f;
		t1.ambient = 0.068f;
		t1.heightFogDensity = 0.0001f;
		t1.fogInscatteringColor = glm::vec3(186.0f / 255.0f, 142.0f / 255.0f, 86.0f / 255.0f);
		t1.lightInscaterringColor = glm::vec3(253.0f / 255.0f, 110.0f / 255.0f, 0.0f);
		t1.skyColor = glm::vec3(0.09f, 0.015f, 0.0f);
		t1.lightShaftsIntensity = 0.31f;

		TimeInfo t2 = {};
		t2.time = 9.5f;
		t2.dirLightColor = glm::vec3(255.0f / 255.0f, 253.0f / 255.0f, 236.0f / 255.0f);
		t2.intensity = 1.4f;
		t2.ambient = 0.141f;
		t2.heightFogDensity = 0.0005f;
		t2.fogInscatteringColor = glm::vec3(138.0f / 255.0f, 195.0f / 255.0f, 250.0f / 255.0f);
		t2.lightInscaterringColor = glm::vec3(255.0f / 255.0f, 252.0f / 255.0f, 170.0f / 255.0f);
		t2.skyColor = glm::vec3(0.10f, 0.17f, 0.23f);
		t2.lightShaftsIntensity = 0.31f;

		// Sunset
		TimeInfo t8 = {};
		t8.time = 17.0f;
		t8.dirLightColor = glm::vec3(255.0f / 255.0f, 253.0f / 255.0f, 236.0f / 255.0f);
		t8.intensity = 1.4f;
		t8.ambient = 0.141f;
		t8.heightFogDensity = 0.0005f;
		t8.fogInscatteringColor = glm::vec3(138.0f / 255.0f, 195.0f / 255.0f, 250.0f / 255.0f);
		t8.lightInscaterringColor = glm::vec3(255.0f / 255.0f, 252.0f / 255.0f, 170.0f / 255.0f);
		t8.skyColor = glm::vec3(0.10f, 0.17f, 0.23f);
		t8.lightShaftsIntensity = 0.31f;

		TimeInfo t4 = {};
		t4.time = 18.05f;
		t4.dirLightColor = glm::vec3(255.0f / 255.0f, 80.0f / 255.0f, 0.0f);
		t4.intensity = 1.25f;
		t4.ambient = 0.068f;
		t4.heightFogDensity = 0.0001f;
		t4.fogInscatteringColor = glm::vec3(186.0f / 255.0f, 142.0f / 255.0f, 86.0f / 255.0f);
		t4.lightInscaterringColor = glm::vec3(253.0f / 255.0f, 110.0f / 255.0f, 0.0f);
		t4.skyColor = glm::vec3(0.09f, 0.015f, 0.0f);
		t4.lightShaftsIntensity = 0.31f;

		TimeInfo t5 = {};
		t5.time = 19.45f;
		t5.dirLightColor = glm::vec3(47.0f / 255.0f, 82.0f / 255.0f, 101.0f / 255.0f);
		t5.intensity = 0.04f;
		t5.ambient = 0.013f;
		t5.heightFogDensity = 0.0001f;
		t5.fogInscatteringColor = glm::vec3(8.0f / 255.0f, 16.0f / 255.0f, 61.0f / 255.0f);
		t5.lightInscaterringColor = glm::vec3(8.0f / 255.0f, 16.0f / 255.0f, 61.0f / 255.0f);
		t5.skyColor = glm::vec3(0.0f, 0.02f, 0.07f);
		t5.lightShaftsIntensity = 0.0f;

		// Midnight
		TimeInfo t6 = {};
		t6.time = 24.0f;
		t6.dirLightColor = glm::vec3(47.0f / 255.0f, 82.0f / 255.0f, 101.0f / 255.0f);
		t6.intensity = 0.04f;
		t6.ambient = 0.013f;
		t6.heightFogDensity = 0.0001f;
		t6.fogInscatteringColor = glm::vec3(8.0f / 255.0f, 16.0f / 255.0f, 61.0f / 255.0f);
		t6.lightInscaterringColor = glm::vec3(8.0f / 255.0f, 16.0f / 255.0f, 61.0f / 255.0f);
		t6.skyColor = glm::vec3(0.0f, 0.02f, 0.07f);
		t6.lightShaftsIntensity = 0.0f;

		timeInfos[0] = t7;
		timeInfos[1] = t1;
		timeInfos[2] = t2;
		timeInfos[3] = t8;
		timeInfos[4] = t4;
		timeInfos[5] = t5;
		timeInfos[6] = t6;

		curIndex = 0;
		nextIndex = 1;
	}

	void TimeOfDayManager::Update(float dt)
	{
		if (Input::IsKeyPressed(KEY_4))
			worldTime += dt * 2.8f;
		if (Input::IsKeyPressed(KEY_5))
			worldTime -= dt * 2.8f;

		//std::cout << worldTime << '\n';

		SetCurrentTime(worldTime);

		//std::cout << a << '\n';

		BlendTimeInfo(timeInfos[curIndex], timeInfos[nextIndex]);
	}

	void TimeOfDayManager::BlendTimeInfo(const TimeInfo &t1, const TimeInfo &t2)
	{
		currentTimeInfo.dirLightColor = glm::mix(t1.dirLightColor, t2.dirLightColor, a);
		currentTimeInfo.ambient = glm::mix(t1.ambient, t2.ambient, a);
		//currentTime.dirLightDirection = glm::mix(t1.dirLightDirection, t2.dirLightDirection, a);
		currentTimeInfo.intensity = glm::mix(t1.intensity, t2.intensity, a);
		currentTimeInfo.heightFogDensity = glm::mix(t1.heightFogDensity, t2.heightFogDensity, a);
		currentTimeInfo.fogInscatteringColor = glm::mix(t1.fogInscatteringColor, t2.fogInscatteringColor, a);
		currentTimeInfo.lightInscaterringColor = glm::mix(t1.lightInscaterringColor, t2.lightInscaterringColor, a);
		currentTimeInfo.skyColor = glm::mix(t1.skyColor, t2.skyColor, a);
		currentTimeInfo.lightShaftsIntensity = glm::mix(t1.lightShaftsIntensity, t2.lightShaftsIntensity, a);
	}

	float TimeOfDayManager::DaysInMonth(int m, int y)
	{
		m = m - 1;
		if (m == 0 || m == 2 || m == 4 || m == 6 || m == 7 || m == 9 || m == 11)
			return 31.0f;
		else if (m == 3 || m == 5 || m == 8 || m == 10)
			return 30.0f;
		else if (m == 1)
		{
			if (y % 4 == 0)
				return 29.0f;
			else if (y % 100 == 0)
				return 28.0f;
			else if (y % 400 == 0)
				return 29.0f;

			return 28.0f;
		}

		return 0.0f;
	}

	void TimeOfDayManager::DayOfYear()
	{
		float sum = 0.0f;
		for (int i = 1; i < month; i++)
		{
			sum += DaysInMonth(i, year);	
		}
		sum += day;
		float h = hour + offset + minute / 60.0f;
		dayOfYear = sum - 1 + (h - 12.0f) / 24.0f;
		//dayOfYear = sum;
	}

	void TimeOfDayManager::CalculateDayValues()
	{
		float x = dayOfYear * 2 * 3.141592653f / 365.0f;			// fractional year in radians
		float eqTime = 229.18f * (0.000075f + 0.001868f * std::cos(x) - 0.032077f * std::sin(x) - 0.014615f * std::cos(2.0f * x) - 0.040849f * std::sin(2.0f * x));

		//std::cout << "Equation of time: " << eqTime << '\n';

		float declin = 0.006918f - 0.399912f * std::cos(x) + 0.070257f * std::sin(x) - 0.006758f * std::cos(2.0f * x);
		declin = declin + 0.000907f * std::sin(2.0f * x) - 0.002697f * std::cos(3.0f * x) + 0.00148f * std::sin(3.0f * x);
		declin = declin * 180 / 3.141592653f;

		//std::cout << "Declination angle: " << declin << '\n';

		// time offset in minutes
		float timeOffset = eqTime - 4.0f * -longi - 60.0f * offset;

		//std::cout << "Time offset in minutes: " << timeOffset << '\n';

		float solarTime = hour * 60.0f + minute + timeOffset;
		//solarTime /= 60.0f;

		//std::cout << "Solar time: " << solarTime / 60.0f << '\n';

		float hourAngle = solarTime / 4.0f - 180.0f;

		//std::cout << "Solar hour angle: " << hourAngle << '\n';

		// Zenith angle
		float k = 3.141592653f / 180.0f ;
		float zenithAngle = std::sin(k * lat) * std::sin(k * declin) + std::cos(k * lat) * std::cos(k * declin) * std::cos(k * hourAngle);
		zenithAngle = std::acos(zenithAngle) / k;
		float altitudeAngle = 90 - zenithAngle;

		//std::cout << "Zenith angle: " << zenithAngle << '\n';
		//std::cout << "Altidude angle: " << altitudeAngle << '\n';

		// Azimuth angle
		float azimuthAngle = -(std::sin(k * lat) * std::cos(k * zenithAngle) - std::sin(k * declin)) / (std::cos(k * lat) * std::sin(k * zenithAngle));
		azimuthAngle = std::acos(azimuthAngle) / k;
		if (hourAngle < 0.0f)
			azimuthAngle = azimuthAngle;
		else
			azimuthAngle = 360.0f - azimuthAngle;

		azimuthAngle += azimuthOffset;

		//std::cout << "Azimuth angle: " << azimuthAngle << '\n';

		currentTimeInfo.dirLightDirection = glm::vec3(std::cos(azimuthAngle*k) * std::cos(altitudeAngle*k), std::sin(altitudeAngle*k), std::sin(azimuthAngle*k) * std::cos(altitudeAngle*k));

		//std::cout << "Sun dir:  x: " << currentTimeInfo.dirLightDirection.x << "  y: " << currentTimeInfo.dirLightDirection.y << "  z: " << currentTimeInfo.dirLightDirection.z << '\n';
	}

	void TimeOfDayManager::CalculateSunriseSunset()
	{
		double zenith = 90.83333333333333;
		double D2R = 3.1415926535897932 / 180.0;
		double R2D = 180.0 / 3.1415926535897932;
		double latitude = 42.545;
		double longitude = -8.428;

		double N1 = floor(275.0 * 2.0 / 9.0);
		double N2 = floor((2.0 + 9.0) / 12.0);
		double N3 = (1.0 + floor((2019.0 - 4.0 * floor(2019.0 / 4.0) + 2.0) / 3.0));
		double day = N1 - (N2 * N3) + 2.0 - 30.0;

		double longHour = longitude / 15.0;
		double tRise = day + ((6.0 - longHour) / 24.0);
		double tSet = day + ((18.0 - longHour) / 24.0);

		// Calculate the Sun's mean anomaly
		double MRise = (0.9856 * tRise) - 3.289;
		double MSet = (0.9856 * tSet) - 3.289;

		// Calculate the Sun's true longitude
		double LRise = MRise + (1.916 * sin(MRise * D2R)) + (0.020 * sin(2 * MRise * D2R)) + 282.634;
		double LSet = MSet + (1.916 * sin(MSet * D2R)) + (0.020 * sin(2 * MSet * D2R)) + 282.634;
		if (LSet > 360.0)
			LSet -= 360.0;
		else if (LSet < 0.0)
			LSet += 360.0;

		if (LSet > 360.0)
			LSet -= 360.0;
		else if (LSet < 0.0)
			LSet += 360.0;

		// Calculate the Sun's right ascension
		double RARise = R2D * atan(0.91764 * tan(LRise * D2R));
		double RASet = R2D * atan(0.91764 * tan(LSet * D2R));
		if (RARise > 360.0)
			RARise -= 360.0;
		else if (RARise < 0.0)
			RARise += 360.0;

		if (RASet > 360.0)
			RASet -= 360.0;
		else if (RASet < 0.0)
			RASet += 360.0;

		// Right ascension value needs to be in the same quadrant as L
		double LquadrantRise = floor(LRise / 90.0) * 90.0;
		double RAquadrantRise = floor(RARise / 90.0) * 90.0;

		double LquadrantSet = floor(LSet / 90.0) * 90.0;
		double RAquadrantSet = floor(RASet / 90.0) * 90.0;

		RARise = RARise + (LquadrantRise - RAquadrantRise);
		RASet = RASet + (LquadrantSet - RAquadrantSet);

		// Right ascension value needs to be converted into hours
		RARise = RARise / 15.0;
		RASet = RASet / 15.0;

		// Calculate the Sun's declination
		double sinDecRise = 0.39782 * sin(LRise * D2R);
		double cosDecRise = cos(asin(sinDecRise));

		double sinDecSet = 0.39782 * sin(LSet * D2R);
		double cosDecSet = cos(asin(sinDecSet));

		// Calculate the Sun's local hour angle
		double cosHRise = (cos(zenith * D2R) - (sinDecRise * sin(latitude * D2R))) / (cosDecRise * cos(latitude * D2R));
		double cosHSet = (cos(zenith * D2R) - (sinDecSet * sin(latitude * D2R))) / (cosDecSet * cos(latitude * D2R));

		double HRise = 360.0 - R2D * acos(cosHRise);
		double HSet = R2D * acos(cosHSet);

		HRise = HRise / 15.0;
		HSet = HSet / 15.0;

		// Calculate local mean time of rising/setting
		double TRise = HRise + RARise - (0.06571 * tRise) - 6.622;
		double TSet = HSet + RASet - (0.06571 * tSet) - 6.622;

		//adjust back to UTC
		double UTRise = TRise - longHour;
		double UTSet = TSet - longHour;

		if (UTRise > 24.0)
			UTRise = UTRise - 24.0;
		else if (UTRise < 0.0)
			UTRise = UTRise + 24.0;

		if (UTSet > 24.0)
			UTSet = UTSet - 24.0;
		else if (UTSet < 0.0)
			UTSet = UTSet + 24.0;

		sunriseTime = (float)UTRise;
		sunsetTime = (float)UTSet;

		// Convert UTC value to local time zone of latitude/longitude
		//float localTSR = utcSR + localOffset;
		//float localTSS = utcSS + localOffset;

		timeInfos[0].time = sunriseTime - 1.1f;
		timeInfos[1].time = sunriseTime;
		timeInfos[2].time = sunriseTime + 1.2f;
		timeInfos[3].time = sunsetTime - 1.05f;
		timeInfos[4].time = sunsetTime;
		timeInfos[5].time = sunsetTime + 1.4f;
		timeInfos[6].time = 24.0f;
	}

	void TimeOfDayManager::Render(Renderer *renderer)
	{
		RenderItem ri = {};
		ri.mesh = &skydomeMesh;
		ri.matInstance = skydomeMat;
		ri.shaderPass = 0;
		renderer->Submit(ri);
	}

	void TimeOfDayManager::Dispose()
	{
		if (transmittanceTexture)
		{
			transmittanceTexture->RemoveReference();
		}

		if (inscatterTexture)
		{
			inscatterTexture->RemoveReference();
		}

		if (skydomeMesh.vao)
		{
			delete skydomeMesh.vao;
			skydomeMesh.vao = nullptr;
		}
	}

	void TimeOfDayManager::SetCurrentTime(float time)
	{
		worldTime = time;
		hour = std::floorf(worldTime);
		minute = (worldTime - (float)((int)worldTime)) * 60.0f;

		DayOfYear();
		CalculateDayValues();
		CalculateSunriseSunset();

		if (worldTime >= 24.0f || worldTime <= 0.0f)
		{
			worldTime = 0.0f;
		}
		float previousTime = 0.0f;
		float nextTime = 0.0f;

		for (size_t i = 0; i < MAX_TIME_INFOS; i++)
		{
			if (i == 0)
				previousTime = timeInfos[MAX_TIME_INFOS -1].time;
			else
				previousTime = timeInfos[i-1].time;

			nextTime = timeInfos[i].time;

			if (worldTime <= nextTime)
			{
				if (i == 0)
					curIndex = MAX_TIME_INFOS - 1;
				else
					curIndex = i - 1;
				
				nextIndex = i;

				if (previousTime == 24.0f)
					previousTime = 0.0f;

				float dif = nextTime - previousTime;		// Eg. nextTime = 18.0, previousTime = 12.0, dif = 6.0, worldTime = 15.0, dif2 = 18.0-15.0=3.0, a = 3/6 = 0.5, halfway through the blend
				float dif2 = nextTime - worldTime;
				a = dif2 / dif;
				a = 1.0f - a;
				break;
			}
		}
	}

	void TimeOfDayManager::SetAzimuthOffset(float offset)
	{
		azimuthOffset = offset;
	}

	void TimeOfDayManager::SetCurrentDay(int day)
	{
		if (day < 1 || day > 31)
			return;

		this->day = (float)day;
		SetCurrentTime(worldTime);
	}

	void TimeOfDayManager::SetCurrentMonth(int month)
	{
		this->month = (float)month;
		SetCurrentTime(worldTime);
	}
}
