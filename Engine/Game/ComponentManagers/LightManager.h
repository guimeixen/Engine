#pragma once

#include "Graphics/Lights.h"
#include "Game/EntityManager.h"

#include "Data/Shaders/common.glsl"

#include <vector>
#include <unordered_map>

namespace Engine
{
	class Buffer;
	class Game;
	class Camera;
	class TransformManager;

	struct PointLightInstance
	{
		Entity e;
		PointLight *pl;
	};

	struct PointLightUBO
	{
		PointLightShader pl[MAX_POINT_LIGHTS];
		int numPointLights;
	};

	class LightManager
	{
	public:
		LightManager();

		//void AddSpotLight(const SpotLight &light);
		void Init(Game *game, TransformManager *transformManager);
		void Update(Camera *mainCamera);
		void PartialDispose();
		void Dispose();	

		PointLight *AddPointLight(Entity e);
		void DuplicatePointLight(Entity e, Entity newE);
		void SetPointLightEnabled(Entity e, bool enable);
		void LoadPointLightFromPrefab(Serializer &s, Entity e);
		void UpdatePointLights();
		void RemoveLight(Entity e);
		bool HasPointLight(Entity e) const;
		PointLight *GetPointLight(Entity e) const;

		const std::vector<LightShader> &GetLightsShaderReady() const { return lightsShaderReady; }
		const PointLightUBO &GetPointLightsUBO() const { return plUBO; }
		unsigned int GetEnabledPointLightsCount() const { return usedPointLights - disabledPointLights; }

		PointLight *CastToPointLight(Light *light) { if (!light) return nullptr; if (light->type == LightType::POINT) { return static_cast<PointLight*>(light); } else { return nullptr; } }

		void Serialize(Serializer &s, bool playMode = false);
		void Deserialize(Serializer &s, bool playMode = false);

	private:
		void InsertPointLightInstance(const PointLightInstance &pli);

	private:
		TransformManager *transformManager;

		std::vector<PointLightInstance> pointLights;						// We can't use this one for sorting otherwise the 
		std::unordered_map<unsigned int, unsigned int> plMap;
		PointLightShader pointLightsShaderReady[MAX_POINT_LIGHTS];
		std::vector<LightShader> lightsShaderReady;

		glm::vec3 lastCamPos;

		PointLightUBO plUBO = {};
		bool needsSort = false;

		unsigned int usedPointLights = 0;
		unsigned int disabledPointLights = 0;
	};
}
