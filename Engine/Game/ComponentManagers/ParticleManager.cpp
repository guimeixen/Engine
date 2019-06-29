#include "ParticleManager.h"

#include "Graphics/ParticleSystem.h"
#include "Game/Game.h"
#include "Graphics/Material.h"
#include "Program/Log.h"

namespace Engine
{
	void ParticleManager::Init(Game *game)
	{
		usedParticleSystems = 0;
		disabledParticleSystems = 0;

		this->game = game;
		transformManager = &game->GetTransformManager();

		Log::Print(LogLevel::LEVEL_INFO, "Init Particle manager\n");
	}

	void ParticleManager::Dispose()
	{
		for (size_t i = 0; i < usedParticleSystems; i++)
		{
			delete particleSystems[i].ps;
		}
		particleSystems.clear();

		Log::Print(LogLevel::LEVEL_INFO, "Disposing Particle manager\n");
	}

	void ParticleManager::Cull(unsigned int passAndFrustumCount, unsigned int *passIds, const Frustum *frustums, std::vector<VisibilityIndices*> &out)
	{
		//const glm::vec3 &camPos = game->GetMainCamera()->GetPosition();

		const unsigned int numEnabledPS = usedParticleSystems - disabledParticleSystems;

		for (unsigned int i = 0; i < passAndFrustumCount; i++)
		{
			for (size_t j = 0; j < numEnabledPS; j++)
			{
				const ParticleInstance &pi = particleSystems[j];

				const AABB &aabb = pi.ps->GetAABB();

				if (frustums[i].BoxInFrustum(aabb.min, aabb.max) != FrustumIntersect::OUTSIDE)
					out[i]->push_back(j);
			}
		}
	}

	void ParticleManager::GetRenderItems(unsigned int passCount, unsigned int *passIds, const VisibilityIndices &visibility, RenderQueue &outQueues)
	{
		float dt = game->GetDeltaTime();
		const unsigned int numEnabledPS = usedParticleSystems - disabledParticleSystems;

		for (size_t i = 0; i < numEnabledPS; i++)
		{
			const ParticleInstance &pi = particleSystems[i];
			ParticleSystem *ps = pi.ps;

			const glm::mat4 &localToWorld = transformManager->GetLocalToWorld(pi.e);

			ps->SetPosition(localToWorld[3]);

			bool renderPrepared = false;
			bool submit = false;

			const Mesh &mesh = ps->GetMesh();
			MaterialInstance *matInstance = ps->GetMaterialInstance();

			const std::vector<ShaderPass> &passes = matInstance->baseMaterial->GetShaderPasses();
			for (size_t j = 0; j < passCount; j++)
			{
				for (size_t k = 0; k < passes.size(); k++)
				{
					if (passIds[j] == passes[k].queueID)
					{
						if (!renderPrepared)
						{
							ps->Update(dt);
							submit = ps->PrepareRender(localToWorld);
							renderPrepared = true;
						}

						if (submit)
						{
							RenderItem ri = {};
							ri.mesh = &mesh;
							ri.matInstance = matInstance;
							ri.shaderPass = k;

							glm::vec4 &params = ps->GetParams();

							ri.materialData = &params;
							ri.materialDataSize = sizeof(params);

							//outQueues[j].push_back(ri);
							outQueues.push_back(ri);
						}
					}
				}
			}
		}
	}

	ParticleSystem *ParticleManager::AddParticleSystem(Entity e)
	{
		if (map.find(e.id) != map.end())
			return particleSystems[map.at(e.id)].ps;

		ParticleSystem *ps = new ParticleSystem();
		ps->Init(game, "Data/Resources/Materials/particlesDefault.mat");
		ps->SetColor(glm::vec4(1.0f, 1.0f, 1.0f, 0.0f));
		ps->SetSize(1.0f);
		ps->SetLifeTime(1.0f);
		ps->SetEmission(10);
		ps->Create(50);

		ps->SetPosition(transformManager->GetLocalToWorld(e)[3]);
		ps->Update(0.0f);		// needed so particle system aabb gets set corretly, otherwise update would not be called because they would not be in the frustum

		ParticleInstance pi = {};
		pi.e = e;
		pi.ps = ps;

		InsertParticleSystem(pi);

		return ps;
	}

	ParticleSystem *ParticleManager::GetParticleSystem(Entity e) const
	{
		return particleSystems[map.at(e.id)].ps;
	}

	void ParticleManager::SetParticleSystemEnabled(Entity e, bool enable)
	{
		if (HasParticleSystem(e))
		{
			if (enable)
			{
				// If we only have one disabled, then there's no need to swap
				if (disabledParticleSystems == 1)
				{
					disabledParticleSystems--;
					return;
				}
				else
				{
					// To enable when there's more than 1 entity disabled just swap the disabled entity that is going to be enabled with the first disabled entity
					unsigned int entityIndex = map.at(e.id);
					unsigned int firstDisabledEntityIndex = usedParticleSystems - disabledParticleSystems;		// Don't subtract -1 because we want the first disabled entity, otherwise we would get the last enabled entity. Eg 6 used, 2 disabled, 6-2=4 the first disabled entity is at index 4 and the second at 5

					ParticleInstance pi1 = particleSystems[entityIndex];
					ParticleInstance pi2 = particleSystems[firstDisabledEntityIndex];

					particleSystems[entityIndex] = pi2;
					particleSystems[firstDisabledEntityIndex] = pi1;

					map[e.id] = firstDisabledEntityIndex;
					map[pi2.e.id] = entityIndex;

					disabledParticleSystems--;
				}
			}
			else
			{
				// Get the indices of the entity to disable and the last entity
				unsigned int entityIndex = map.at(e.id);
				unsigned int firstDisabledEntityIndex = usedParticleSystems - disabledParticleSystems - 1;		// Get the first entity disabled or the last entity if none are disabled

				ParticleInstance pi1 = particleSystems[entityIndex];
				ParticleInstance pi2 = particleSystems[firstDisabledEntityIndex];

				// Now swap the entities
				particleSystems[entityIndex] = pi2;
				particleSystems[firstDisabledEntityIndex] = pi1;

				// Swap the indices
				map[e.id] = firstDisabledEntityIndex;
				map[pi2.e.id] = entityIndex;

				disabledParticleSystems++;
			}
		}
	}

	void ParticleManager::DuplicateParticleSystem(Entity e, Entity newE)
	{
		if (HasParticleSystem(e) == false)
			return;

		const ParticleSystem *ps = GetParticleSystem(e);

		ParticleSystem *newPS = new ParticleSystem();
		newPS->Init(game, ps->GetMaterialInstance()->path);
		newPS->Create(ps->GetMaxParticles());
		newPS->SetSize(ps->GetSize());
		newPS->SetEmission(ps->GetEmission());
		newPS->SetLifeTime(ps->GetLifetime());
		newPS->SetIsLooping(ps->IsLooping());
		newPS->SetDuration(ps->GetDuration());		
		newPS->SetCenter(ps->GetCenter());
		newPS->SetEmissionBox(ps->GetEmissionBoxSize());
		newPS->SetEmissionRadius(ps->GetEmissionRadius());
		newPS->SetColor(ps->GetColor());
		newPS->SetFadeAlphaOverLifetime(ps->GetFadeAlphaOverLifeTime());
		newPS->SetLimitVelocityOverLifeTime(ps->GetLimitVelocityOverLifetime());
		newPS->SetSpeedLimit(ps->GetSpeedLimit());
		newPS->SetDampen(ps->GetDampen());
		newPS->SetGravityModifier(ps->GetGravityModifier());
		newPS->SetUseAtlas(ps->UsesAtlas());

		const Engine::AtlasInfo &info = ps->GetAtlasInfo();
		newPS->SetAtlasInfo(info.nColumns, info.nRows, info.cycles);

		if (ps->UsesRandomVelocity())
			newPS->SetRandomVelocity(ps->GetRandomVelocityLow(), ps->GetRandomVelocityHigh());
		else
			newPS->SetVelocity(ps->GetVelocity());

		newPS->SetPosition(transformManager->GetLocalToWorld(newE)[3]);
		newPS->Update(0.0f);		// needed so particle system aabb gets set corretly, otherwise update would not be called because they would not be in the frustum

		ParticleInstance pi;
		pi.e = newE;
		pi.ps = newPS;

		InsertParticleSystem(pi);
	}

	void ParticleManager::LoadParticleSystemFromPrefab(Serializer &s, Entity e)
	{
		ParticleInstance pi = {};
		pi.e = e;

		std::string matPath;
		s.Read(matPath);

		pi.ps = new ParticleSystem();
		pi.ps->Deserialize(s);
		pi.ps->Init(game, matPath);
		pi.ps->Create(pi.ps->GetMaxParticles());

		InsertParticleSystem(pi);
	}

	void ParticleManager::InsertParticleSystem(const ParticleInstance &pi)
	{
		if (usedParticleSystems < particleSystems.size())
		{
			particleSystems[usedParticleSystems] = pi;
			map[pi.e.id] = usedParticleSystems;
		}
		else
		{
			particleSystems.push_back(pi);
			map[pi.e.id] = (unsigned int)particleSystems.size() - 1;
		}

		usedParticleSystems++;

		// If there is any disabled entity then we need to swap the new one, which was inserted at the end, with the first disabled entity
		if (disabledParticleSystems > 0)
		{
			// Get the indices of the entity to disable and the last entity
			unsigned int newEntityIndex = usedParticleSystems - 1;
			unsigned int firstDisabledEntityIndex = usedParticleSystems - disabledParticleSystems - 1;		// Get the first entity disabled

			ParticleInstance pi1 = particleSystems[newEntityIndex];
			ParticleInstance pi2 = particleSystems[firstDisabledEntityIndex];

			// Now swap the entities
			particleSystems[newEntityIndex] = pi2;
			particleSystems[firstDisabledEntityIndex] = pi1;

			// Swap the indices
			map[pi.e.id] = firstDisabledEntityIndex;
			map[pi2.e.id] = newEntityIndex;
		}
	}

	void ParticleManager::RemoveParticleSystem(Entity e)
	{
		if (HasParticleSystem(e))
		{
			// To remove an entity we need to swap it with the last one, but, because there could be disabled entities at the end
			// we need to first swap the entity to remove with the last ENABLED entity and then if there are any disabled entities, swap the entity to remove again,
			// but this time with the last disabled entity.
			unsigned int entityToRemoveIndex = map.at(e.id);
			unsigned int lastEnabledEntityIndex = usedParticleSystems - disabledParticleSystems - 1;

			ParticleInstance entityToRemovePi = particleSystems[entityToRemoveIndex];
			ParticleInstance lastEnabledEntityPi = particleSystems[lastEnabledEntityIndex];

			// Swap the entity to remove with the last enabled entity
			particleSystems[lastEnabledEntityIndex] = entityToRemovePi;
			particleSystems[entityToRemoveIndex] = lastEnabledEntityPi;

			// Now change the index of the last enabled entity, which is now in the spot of the entity to remove, to the entity to remove index
			map[lastEnabledEntityPi.e.id] = entityToRemoveIndex;
			map.erase(e.id);

			// If there any disabled entities then swap the entity to remove, which is is the spot of the last enabled entity, with the last disabled entity
			if (disabledParticleSystems > 0)
			{
				entityToRemoveIndex = lastEnabledEntityIndex;			// The entity to remove is now in the spot of the last enabled entity
				unsigned int lastDisabledEntityIndex = usedParticleSystems - 1;

				ParticleInstance lastDisabledEntityPi = particleSystems[lastDisabledEntityIndex];

				particleSystems[lastDisabledEntityIndex] = entityToRemovePi;
				particleSystems[entityToRemoveIndex] = lastDisabledEntityPi;

				map[lastDisabledEntityPi.e.id] = entityToRemoveIndex;
			}

			delete entityToRemovePi.ps;
			usedParticleSystems--;
		}
	}

	bool ParticleManager::HasParticleSystem(Entity e) const
	{
		return map.find(e.id) != map.end();
	}

	void ParticleManager::Serialize(Serializer &s, bool playMode)
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

		s.Write(usedParticleSystems);
		s.Write(disabledParticleSystems);
		for (unsigned int i = 0; i < usedParticleSystems; i++)
		{
			const ParticleInstance &pi = particleSystems[i];
			s.Write(pi.e.id);
			pi.ps->Serialize(s);
		}
	}

	void ParticleManager::Deserialize(Serializer &s, bool playMode)
	{
		Log::Print(LogLevel::LEVEL_INFO, "Deserializing particle manager\n");

		if (!playMode)
		{
			s.Read(usedParticleSystems);
			s.Read(disabledParticleSystems);
			particleSystems.resize(usedParticleSystems);
			std::string matPath;
			for (unsigned int i = 0; i < usedParticleSystems; i++)
			{
				ParticleInstance pi;
				s.Read(pi.e.id);

				s.Read(matPath);
				pi.ps = new ParticleSystem();
				pi.ps->Deserialize(s);
				pi.ps->Init(game, matPath);
				pi.ps->Create(pi.ps->GetMaxParticles());

				particleSystems[i] = pi;
				map[pi.e.id] = i;
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

			s.Read(usedParticleSystems);
			s.Read(disabledParticleSystems);

			std::string matPath;

			for (unsigned int i = 0; i < usedParticleSystems; i++)
			{
				s.Read(eid);
				unsigned int idx = map[eid];

				ParticleInstance &pi = particleSystems[idx];
				pi.e.id = eid;
				pi.ps->Deserialize(s);
			}		
		}
	}
}
