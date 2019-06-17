#include "ParticleManager.h"

#include "Graphics/ParticleSystem.h"
#include "Game/Game.h"
#include "Graphics/Material.h"
#include "Program/Log.h"

namespace Engine
{
	void ParticleManager::Init(Game *game)
	{
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

		for (unsigned int i = 0; i < passAndFrustumCount; i++)
		{
			for (size_t j = 0; j < particleSystems.size(); j++)
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

		for (size_t i = 0; i < usedParticleSystems; i++)
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

		if (usedParticleSystems < particleSystems.size())
		{
			particleSystems[usedParticleSystems] = pi;
			map[e.id] = usedParticleSystems;
		}
		else
		{
			particleSystems.push_back(pi);
			map[e.id] = (unsigned int)particleSystems.size() - 1;
		}	

		usedParticleSystems++;

		return ps;
	}

	ParticleSystem *ParticleManager::GetParticleSystem(Entity e) const
	{
		return particleSystems[map.at(e.id)].ps;
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

		if (usedParticleSystems < particleSystems.size())
		{
			particleSystems[usedParticleSystems] = pi;
			map[newE.id] = usedParticleSystems;
		}
		else
		{
			particleSystems.push_back(pi);
			map[newE.id] = (unsigned int)particleSystems.size() - 1;
		}

		usedParticleSystems++;

	}

	void ParticleManager::RemoveParticleSystem(Entity e)
	{
		if (HasParticleSystem(e))
		{
			unsigned int index = map.at(e.id);

			ParticleInstance temp = particleSystems[index];
			ParticleInstance last = particleSystems[particleSystems.size() - 1];
			particleSystems[particleSystems.size() - 1] = temp;
			particleSystems[index] = last;

			map[last.e.id] = index;
			map.erase(e.id);

			delete temp.ps;
			usedParticleSystems--;
		}
	}

	bool ParticleManager::HasParticleSystem(Entity e) const
	{
		return map.find(e.id) != map.end();
	}

	void ParticleManager::Serialize(Serializer &s)
	{
		s.Write(usedParticleSystems);
		for (unsigned int i = 0; i < usedParticleSystems; i++)
		{
			const ParticleInstance &pi = particleSystems[i];
			s.Write(pi.e.id);
			pi.ps->Serialize(s);
		}
	}

	void ParticleManager::Deserialize(Serializer &s, bool reload)
	{
		if (!reload)
		{
			s.Read(usedParticleSystems);
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
			s.Read(usedParticleSystems);
			std::string matPath;
			for (unsigned int i = 0; i < usedParticleSystems; i++)
			{
				ParticleInstance &pi = particleSystems[i];
				s.Read(pi.e.id);
				pi.ps->Deserialize(s);
			}
		}
	}
}
