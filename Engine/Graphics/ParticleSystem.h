#pragma once

#include "Mesh.h"
#include "Physics\BoundingVolumes.h"
#include "Program\Serializer.h"

#include "include\glm\glm.hpp"

#include <string>
#include <vector>
#include <fstream>

namespace Engine
{
	struct MaterialInstance;
	class Texture;
	class Buffer;
	class Game;
	class Renderer;

	enum EmissionShape
	{
		BOX,
		SPHERE
	};

	struct PSInstanceData
	{
		glm::vec4 posBlendFactor;
		glm::vec4 color;
		glm::vec4 texOffsets;
		//glm::vec4 rotation;
	};

	struct AtlasInfo
	{
		int nColumns;
		int nRows;
		float cycles;			// How many times the texture atlas will be looped in the lifetime of a particle
	};

	struct Particle
	{
		glm::vec3 position;
		glm::vec3 velocity;
		glm::vec4 color;
		//float zRotation;
		float life;
		float blendFactor;
		glm::vec2 texOffset1;
		glm::vec2 texOffset2;
	};

	class ParticleSystem
	{
	private:
		friend class ParticleManager;
	public:
		ParticleSystem();
		~ParticleSystem();

		void Init(Game *game, const std::string &matPath);
		void Create(int maxParticles);
		void Update(float dt);
		bool PrepareRender(const glm::mat4 &transform);

		void SetMaterialInstance(MaterialInstance *mat);
		void SetCenter(const glm::vec3 &center);
		void SetPosition(const glm::vec3 &position) { worldPos = position; }
		void SetVelocity(const glm::vec3 &velocity) { startVelocity = velocity; useRandomVelocity = false; }
		void SetRandomVelocity(const glm::vec3 &low, const glm::vec3 &high);
		void SetColor(const glm::vec4 &color) { startColor = color; }
		void SetSize(float size);
		void SetLifeTime(float lifeTime) { startLifeTime = lifeTime; }
		void SetEmission(int emission) { this->emission = emission; }
		void SetAtlasInfo(int columns, int rows, float cycles);
		void SetIsLooping(bool loop) { isLooping = loop; }
		void SetDuration(float duration) { this->duration = duration; }
		void SetUseAtlas(bool use) { useAtlas = use; }
		void SetMaxParticles(unsigned int maxParticles);
		void SetEmissionBox(const glm::vec3 &box);
		void SetEmissionRadius(float radius);
		void SetFadeAlphaOverLifetime(bool fade) { fadeAlphaOverLifetime = fade; }

		void SetGravityModifier(float value) { gravityModifier = value; }

		// Limit velocity over lifetime
		void SetLimitVelocityOverLifeTime(bool limit) { limitVelocity = limit; }
		void SetSpeedLimit(float limit) { speedLimit = limit; }
		void SetDampen(float dampen) { this->dampen = dampen; }

		const Mesh &GetMesh() const { return quadMesh; }
		MaterialInstance *GetMaterialInstance() const { return matInstance; }
		glm::vec4 &GetParams();
		float GetLifetime() const { return startLifeTime; }
		float GetSize() const { return startSize; }
		float GetEmissionRadius() const { return emissionRadius; }
		float GetDuration() const { return duration; }
		const glm::vec4 &GetColor() const { return startColor; }
		bool UsesRandomVelocity() const { return useRandomVelocity; }

		int GetMaxParticles() const { return maxParticles; }
		int GetEmission() const { return emission; }

		bool UsesAtlas() const { return useAtlas; }
		bool IsLooping() const { return isLooping; }
		bool GetFadeAlphaOverLifeTime() const { return fadeAlphaOverLifetime; }

		bool GetLimitVelocityOverLifetime() const { return limitVelocity; }
		float GetSpeedLimit()const { return speedLimit; }
		float GetDampen() const { return dampen; }

		float GetGravityModifier() const { return gravityModifier; }

		const glm::vec3 &GetCenter() const { return center; }
		const glm::vec3 &GetEmissionBoxSize() const { return emissionBox; }
		const glm::vec3 &GetVelocity() const { return startVelocity; }
		const glm::vec3 &GetRandomVelocityLow() const { return velocityLow; }
		const glm::vec3 &GetRandomVelocityHigh() const { return velocityHigh; }
		const AtlasInfo &GetAtlasInfo() const { return atlasInfo; }
		const AABB &GetAABB() const { return aabb; }

		void Serialize(Serializer &s);
		void Deserialize(Serializer &s);

		// Script functions
		void Stop();
		void Play();

	private:
		void RespawnParticle(Particle &particle);
		int FirstUnusedParticle();
		glm::vec2 SetTextureOffset(int index);

	private:
		Renderer * renderer;
		unsigned int id;
		MaterialInstance *matInstance;
		bool materialUBOChanged;
		Mesh quadMesh;
		AABB aabb;
		AtlasInfo atlasInfo;
		std::vector<PSInstanceData> instanceData;
		Buffer *instanceVB;
		glm::vec4 params;
		unsigned int maxParticles;

		glm::vec3 worldPos;
		glm::vec3 center;
		float timePlaying = 0.0f;
		float duration = 1.0f;
		bool playing = true;
		bool isLooping = true;
		bool particlesAlive = false;
		bool fadeAlphaOverLifetime = true;
		//bool useGlobalRotation = true;
		//float zRotation = 0.0f;

		int emissionShape;
		glm::vec3 emissionBox;
		float emissionRadius;

		int lastUsedParticle;
		int emission;
		float accumulator;

		//glm::vec3 startPosition;
		glm::vec3 startVelocity;
		glm::vec4 startColor;
		float startSize;
		float startLifeTime;

		bool useRandomVelocity;
		glm::vec3 velocityLow;
		glm::vec3 velocityHigh;

		bool useAtlas;

		bool limitVelocity = false;
		float speedLimit = 0.0f;
		float dampen = 1.0f;

		float gravityModifier = 0.0f;

		std::vector<Particle> particles;
	};
}
