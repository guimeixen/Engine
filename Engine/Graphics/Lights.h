#pragma once

#include "Program/Serializer.h"

namespace Engine
{
	enum class LightType
	{
		DIRECTIONAL,
		POINT,
		SPOT
	};

	struct Light
	{
		LightType type;
		float intensity;
		glm::vec3 color;
		bool castShadows;

		virtual void Serialize(Serializer &s)
		{
			s.Write(static_cast<int>(type));
			s.Write(intensity);
			s.Write(color);
			s.Write(castShadows);
		}
		virtual void Deserialize(Serializer &s)
		{
			s.Read(intensity);
			s.Read(color);
			s.Read(castShadows);
		}

		virtual ~Light(){}
	};

	struct DirLight : public Light
	{
		DirLight() { type = LightType::DIRECTIONAL; }
		glm::vec3 direction;
		float ambient;

		void Serialize(Serializer &s)
		{
			Light::Serialize(s);
			s.Write(direction);
			s.Write(ambient);
		}
		void Deserialize(Serializer &s)
		{
			Light::Deserialize(s);
			s.Read(direction);
		}
	};

	struct PointLight : public Light
	{
		PointLight() { type = LightType::POINT; }
		glm::vec3 position;
		glm::vec3 positionOffset;
		float radius;

		void Serialize(Serializer &s)
		{
			Light::Serialize(s);
			s.Write(position);
			s.Write(radius);
			s.Write(positionOffset);
		}
		void Deserialize(Serializer &s)
		{
			Light::Deserialize(s);
			s.Read(position);
			s.Read(radius);
			s.Read(positionOffset);
		}
	};

	struct PointLightShader
	{
		glm::vec4 posAndIntensity;
		glm::vec4 colorAndRadius;
	};

	struct LightShader
	{
		//unsigned int type;
		glm::vec4 posAndIntensityWS;
		glm::vec4 posAndIntensityVS;
		glm::vec4 colorAndRadius;
	};
}
