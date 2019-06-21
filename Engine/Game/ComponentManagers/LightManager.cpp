#include "LightManager.h"

#include "Graphics/Buffers.h"
#include "Program/Utils.h"
#include "Game/Game.h"
#include "Game/ComponentManagers/TransformManager.h"
#include "Program/Log.h"

#include "include/glm/gtc/matrix_transform.hpp"
#include "include/glm/gtx/norm.hpp"

namespace Engine
{
	LightManager::LightManager()
	{
		lastCamPos = glm::vec3(0.0f);
	}

	void LightManager::Init(Game *game, TransformManager *transformManager)
	{
		this->transformManager = transformManager;

		Log::Print(LogLevel::LEVEL_INFO, "Init Light manager\n");
	}

	void LightManager::Update(Camera *mainCamera)
	{
		for (unsigned int i = 0; i < usedPointLights; i++)
		{
			PointLightInstance &pli = pointLights[i];
			pli.pl->position = transformManager->GetLocalToWorld(pli.e)[3];
			needsSort = true;
		}

		glm::vec3 camPos = mainCamera->GetPosition();

		// Only sort the point lights if the camera moves
		float absX = std::fabs(lastCamPos.x - camPos.x);
		float absY = std::fabs(lastCamPos.y - camPos.y);
		float absZ = std::fabs(lastCamPos.z - camPos.z);
		if (needsSort || absX > 0.01f || absY > 0.01f || absZ > 0.01f)
		{
			std::sort(pointLights.begin(), pointLights.end(), [camPos](const PointLightInstance &a, const PointLightInstance &b) -> bool
			{
				return glm::length2(camPos - a.pl->position + a.pl->positionOffset) < glm::length2(camPos - b.pl->position + b.pl->positionOffset);
			});

			unsigned int size = usedPointLights > MAX_POINT_LIGHTS ? MAX_POINT_LIGHTS : usedPointLights;

			plUBO.numPointLights = (int)size;

			short numShadowCastingPL = 0;

			for (unsigned int i = 0; i < size; i++)
			{
				const PointLight *pl = pointLights[i].pl;
				PointLightShader &pls = pointLightsShaderReady[i];
				pls.posAndIntensity = glm::vec4(pl->position + pl->positionOffset, pl->intensity);
				pls.colorAndRadius = glm::vec4(pl->color, pl->radius);

				if (pl->castShadows)
				{
					// create render queue

					numShadowCastingPL++;
				}

				plUBO.pl[i] = pls;
			}

			//pointLightUBO->Update(&plUBO, sizeof(plUBO), 0);

			lastCamPos = camPos;
			needsSort = false;
		}

		if (usedPointLights > lightsShaderReady.size())
			lightsShaderReady.resize(usedPointLights);

		const glm::mat4 &view = mainCamera->GetViewMatrix();
		glm::mat4 t;
		for (size_t i = 0; i < lightsShaderReady.size(); i++)
		{
			const PointLight *pl = pointLights[i].pl;
			LightShader &l = lightsShaderReady[i];
			l.posAndIntensityWS = glm::vec4(pl->position + pl->positionOffset, pl->intensity);
			l.colorAndRadius = glm::vec4(pl->color, pl->radius);

			t = view * glm::translate(glm::mat4(1.0f), glm::vec3(l.posAndIntensityWS.x, l.posAndIntensityWS.y, l.posAndIntensityWS.z));
			l.posAndIntensityVS = t[3];
		}
	}

	void LightManager::PartialDispose()
	{
		for (size_t i = 0; i < usedPointLights; i++)
			delete pointLights[i].pl;

		pointLights.clear();
	}

	void LightManager::Dispose()
	{
		PartialDispose();

		Log::Print(LogLevel::LEVEL_INFO, "Disposing Light manager\n");
	}

	PointLight *LightManager::AddPointLight(Entity e)
	{
		if (HasPointLight(e))
			return pointLights[plMap.at(e.id)].pl;

		PointLight *light = new PointLight();
		light->type = LightType::POINT;
		light->castShadows = false;
		light->color = glm::vec3(1.0f);
		light->intensity = 1.0f;
		light->radius = 10.0f;
		light->positionOffset = glm::vec3(0.0f);

		PointLightInstance pli = {};
		pli.e = e;
		pli.pl = light;

		if (usedPointLights < pointLights.size())
		{
			pointLights[usedPointLights] = pli;
			plMap[e.id] = usedPointLights;
		}
		else
		{
			pointLights.push_back(pli);
			plMap[e.id] = (unsigned int)pointLights.size() - 1;
		}

		usedPointLights++;

		return light;
	}

	void LightManager::DuplicatePointLight(Entity e, Entity newE)
	{
		if (HasPointLight(e) == false)
			return;

		const PointLight *pl = GetPointLight(e);

		PointLight *newPl = new PointLight();
		newPl->type = LightType::POINT;
		newPl->castShadows = pl->castShadows;
		newPl->color = pl->color;
		newPl->intensity = pl->intensity;
		newPl->radius = pl->radius;
		newPl->positionOffset = pl->positionOffset;

		PointLightInstance pli;
		pli.e = newE;
		pli.pl = newPl;

		if (usedPointLights < pointLights.size())
		{
			pointLights[usedPointLights] = pli;
			plMap[newE.id] = usedPointLights;
		}
		else
		{
			pointLights.push_back(pli);
			plMap[newE.id] = (unsigned int)pointLights.size() - 1;
		}

		usedPointLights++;
	}

	void LightManager::EnablePointLight(PointLight *light)
	{
		for (size_t i = 0; i < usedPointLights; i++)
		{
			if (pointLights[i].pl == light)			// If we already have the light don't readd it
				return;
		}

		//pointLights.push_back(light);
	}

	void LightManager::UpdatePointLights()
	{
		needsSort = true;
	}

	void LightManager::RemoveLight(Entity e)
	{
		if (HasPointLight(e))
		{
			unsigned int index = plMap.at(e.id);

			PointLightInstance temp = pointLights[index];
			PointLightInstance last = pointLights[pointLights.size() - 1];
			pointLights[pointLights.size() - 1] = temp;
			pointLights[index] = last;

			plMap[last.e.id] = index;
			plMap.erase(e.id);

			delete temp.pl;
			needsSort = true;
			usedPointLights--;
		}
	}

	void LightManager::RemovePointLight(PointLight *light)
	{
		int index = 0;
		for (auto it = pointLights.begin(); it != pointLights.end(); it++)
		{
			if ((*it).pl == light)
			{
				pointLights.erase(it);
				break;
			}
			++index;
		}
	}

	bool LightManager::HasPointLight(Entity e) const
	{
		return plMap.find(e.id) != plMap.end();
	}

	PointLight *LightManager::GetPointLight(Entity e) const
	{
		return pointLights[plMap.at(e.id)].pl;
	}

	void LightManager::Serialize(Serializer &s)
	{
		s.Write(usedPointLights);
		for (unsigned int i = 0; i < usedPointLights; i++)
		{
			const PointLightInstance &pli = pointLights[i];
			s.Write(pli.e.id);
			pli.pl->Serialize(s);
		}
	}

	void LightManager::Deserialize(Serializer &s, bool reload)
	{
		Log::Print(LogLevel::LEVEL_INFO, "Deserializing light manager\n");

		if (!reload)
		{
			s.Read(usedPointLights);
			pointLights.resize(usedPointLights);
			for (unsigned int i = 0; i < usedPointLights; i++)
			{
				PointLightInstance pli;
				s.Read(pli.e.id);
				pli.pl = new PointLight();
				pli.pl->Deserialize(s);

				pointLights[i] = pli;
				plMap[pli.e.id] = i;
			}
		}
		else
		{
			s.Read(usedPointLights);
			for (unsigned int i = 0; i < usedPointLights; i++)
			{
				PointLightInstance &pli = pointLights[i];
				s.Read(pli.e.id);
				pli.pl->Deserialize(s);
			}
		}
	}
}
