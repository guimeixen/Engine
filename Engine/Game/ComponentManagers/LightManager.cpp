#include "LightManager.h"

#include "Graphics/Buffers.h"
#include "Program/Utils.h"
#include "Game/Game.h"
#include "Game/ComponentManagers/TransformManager.h"
#include "Program/Log.h"

#include "include/glm/gtc/matrix_transform.hpp"
#include "include/glm/gtx/norm.hpp"

#include <algorithm>

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
		const unsigned int numEnabledPointLights = usedPointLights - disabledPointLights;

		for (unsigned int i = 0; i < numEnabledPointLights; i++)
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
			// We can't sort the actual pointLights list because it would mess up the map indices and disabled points lights
			// We would need to sort the shader light lists instead of this one
			/*std::sort(pointLights.begin(), pointLights.end() - disabledPointLights, [camPos](const PointLightInstance &a, const PointLightInstance &b) -> bool
			{
				return glm::length2(camPos - a.pl->position + a.pl->positionOffset) < glm::length2(camPos - b.pl->position + b.pl->positionOffset);
			});*/

			unsigned int size = numEnabledPointLights > MAX_POINT_LIGHTS ? MAX_POINT_LIGHTS : numEnabledPointLights;

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

		if (numEnabledPointLights > lightsShaderReady.size())
			lightsShaderReady.resize(numEnabledPointLights);

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

		InsertPointLightInstance(pli);

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

		InsertPointLightInstance(pli);
	}

	void LightManager::SetPointLightEnabled(Entity e, bool enable)
	{
		if (HasPointLight(e))
		{
			// We need to rebuild the points lights list
			lightsShaderReady.clear();

			if (enable)
			{
				// If we only have one disabled, then there's no need to swap
				if (disabledPointLights == 1)
				{
					disabledPointLights--;
					return;
				}
				else
				{
					// To enable when there's more than 1 entity disabled just swap the disabled entity that is going to be enabled with the first disabled entity
					unsigned int entityIndex = plMap.at(e.id);
					unsigned int firstDisabledEntityIndex = usedPointLights - disabledPointLights;		// Don't subtract -1 because we want the first disabled entity, otherwise we would get the last enabled entity. Eg 6 used, 2 disabled, 6-2=4 the first disabled entity is at index 4 and the second at 5

					PointLightInstance pli1 = pointLights[entityIndex];
					PointLightInstance pli2 = pointLights[firstDisabledEntityIndex];

					pointLights[entityIndex] = pli2;
					pointLights[firstDisabledEntityIndex] = pli1;

					plMap[e.id] = firstDisabledEntityIndex;
					plMap[pli2.e.id] = entityIndex;

					disabledPointLights--;
				}
			}
			else
			{
				// Get the indices of the entity to disable and the last entity
				unsigned int entityIndex = plMap.at(e.id);
				unsigned int firstDisabledEntityIndex = usedPointLights - disabledPointLights - 1;		// Get the first entity disabled or the last entity if none are disabled

				PointLightInstance pli1 = pointLights[entityIndex];
				PointLightInstance pli2 = pointLights[firstDisabledEntityIndex];

				// Now swap the entities
				pointLights[entityIndex] = pli2;
				pointLights[firstDisabledEntityIndex] = pli1;

				// Swap the indices
				plMap[e.id] = firstDisabledEntityIndex;
				plMap[pli2.e.id] = entityIndex;

				disabledPointLights++;
			}
		}
	}

	void LightManager::LoadPointLightFromPrefab(Serializer &s, Entity e)
	{
		PointLightInstance pli = {};
		pli.e = e;
		pli.pl = new PointLight();
		pli.pl->Deserialize(s);

		InsertPointLightInstance(pli);
	}

	void LightManager::InsertPointLightInstance(const PointLightInstance &pli)
	{
		if (usedPointLights < pointLights.size())
		{
			pointLights[usedPointLights] = pli;
			plMap[pli.e.id] = usedPointLights;
		}
		else
		{
			pointLights.push_back(pli);
			plMap[pli.e.id] = (unsigned int)pointLights.size() - 1;
		}

		usedPointLights++;

		// If there is any disabled entity then we need to swap the new one, which was inserted at the end, with the first disabled entity
		if (disabledPointLights > 0)
		{
			// Get the indices of the entity to disable and the last entity
			unsigned int newEntityIndex = usedPointLights - 1;
			unsigned int firstDisabledEntityIndex = usedPointLights - disabledPointLights - 1;		// Get the first entity disabled

			PointLightInstance pli1 = pointLights[newEntityIndex];
			PointLightInstance pli2 = pointLights[firstDisabledEntityIndex];

			// Now swap the entities
			pointLights[newEntityIndex] = pli2;
			pointLights[firstDisabledEntityIndex] = pli1;

			// Swap the indices
			plMap[pli.e.id] = firstDisabledEntityIndex;
			plMap[pli2.e.id] = newEntityIndex;
		}
	}

	void LightManager::UpdatePointLights()
	{
		needsSort = true;
	}

	void LightManager::RemoveLight(Entity e)
	{
		if (HasPointLight(e))
		{
			// To remove an entity we need to swap it with the last one, but, because there could be disabled entities at the end
			// we need to first swap the entity to remove with the last ENABLED entity and then if there are any disabled entities, swap the entity to remove again,
			// but this time with the last disabled entity.
			unsigned int entityToRemoveIndex = plMap.at(e.id);
			unsigned int lastEnabledEntityIndex = usedPointLights - disabledPointLights - 1;

			PointLightInstance entityToRemovePli = pointLights[entityToRemoveIndex];
			PointLightInstance lastEnabledEntityPli = pointLights[lastEnabledEntityIndex];

			// Swap the entity to remove with the last enabled entity
			pointLights[lastEnabledEntityIndex] = entityToRemovePli;
			pointLights[entityToRemoveIndex] = lastEnabledEntityPli;

			// Now change the index of the last enabled entity, which is now in the spot of the entity to remove, to the entity to remove index
			plMap[lastEnabledEntityPli.e.id] = entityToRemoveIndex;
			plMap.erase(e.id);

			// If there any disabled entities then swap the entity to remove, which is is the spot of the last enabled entity, with the last disabled entity
			if (disabledPointLights > 0)
			{
				entityToRemoveIndex = lastEnabledEntityIndex;			// The entity to remove is now in the spot of the last enabled entity
				unsigned int lastDisabledEntityIndex = usedPointLights - 1;

				PointLightInstance lastDisabledEntityPli = pointLights[lastDisabledEntityIndex];

				pointLights[lastDisabledEntityIndex] = entityToRemovePli;
				pointLights[entityToRemoveIndex] = lastDisabledEntityPli;

				plMap[lastDisabledEntityPli.e.id] = entityToRemoveIndex;
			}

			delete entityToRemovePli.pl;
			needsSort = true;
			usedPointLights--;
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
		s.Write(disabledPointLights);
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
			s.Read(disabledPointLights);
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
			s.Read(disabledPointLights);
			for (unsigned int i = 0; i < usedPointLights; i++)
			{
				PointLightInstance &pli = pointLights[i];
				s.Read(pli.e.id);
				pli.pl->Deserialize(s);
			}
		}
	}
}
