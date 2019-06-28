#pragma once

#include "Game/EntityManager.h"
#include "Graphics/RendererStructs.h"

#include <vector>
#include <unordered_map>

namespace Engine
{
	class ParticleSystem;
	class Game;
	class TransformManager;

	struct ParticleInstance
	{
		Entity e;
		ParticleSystem *ps;
	};

	class ParticleManager : public RenderQueueGenerator
	{
	public:
		void Init(Game *game);
		void Dispose();

		void Cull(unsigned int passAndFrustumCount, unsigned int *passIds, const Frustum *frustums, std::vector<VisibilityIndices*> &out) override;
		void GetRenderItems(unsigned int passCount, unsigned int *passIds, const VisibilityIndices &visibility, RenderQueue &outQueues) override;

		ParticleSystem *AddParticleSystem(Entity e);
		ParticleSystem *GetParticleSystem(Entity e) const;
		void SetParticleSystemEnabled(Entity e, bool enable);
		void DuplicateParticleSystem(Entity e, Entity newE);
		void LoadParticleSystemFromPrefab(Serializer &s, Entity e);
		void RemoveParticleSystem(Entity e);
		bool HasParticleSystem(Entity e) const;

		void Serialize(Serializer &s);
		void Deserialize(Serializer &s, bool reload = false);

	private:
		void InsertParticleSystem(const ParticleInstance &pi);

	private:
		Game *game;
		TransformManager *transformManager;
		std::vector<ParticleInstance> particleSystems;
		std::unordered_map<unsigned int, unsigned int> map;
		unsigned int usedParticleSystems;
		unsigned int disabledParticleSystems;
	};
}
