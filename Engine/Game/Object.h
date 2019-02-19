#pragma once

#include "Game\ComponentManagers\PhysicsManager.h"
#include "Physics\BoundingVolumes.h"
#include "Game\ComponentManagers\ScriptManager.h"
#include "Game\Transform.h"
#include "Graphics\Lights.h"

#include <string>

namespace Engine
{
	class Game;
	class Renderer;

	class Object : public Transform
	{
	public:
		Object(Game *game, unsigned int id);
		virtual ~Object() {}

		bool IsDirty() const { return isDirty || transformChanged; }
		void CopyTo(Object *dstObj);
		bool Equals(Object *obj) const { return obj == this; }

		void SetLayer(int layer);
		int GetLayer() const { return layer; }

		void SetLocalPosition(const glm::vec3 &position);
		void SetLocalRotation(const glm::quat &rot);
		void SetLocalScale(const glm::vec3 &scale);
		void SetEnabled(bool enabled);

		bool IsEnabled() const { return isEnabled; }

		void Serialize(Serializer &s);
		void Deserialize(Serializer &s);

	protected:
		bool transformChanged;
		bool isEnabled = true;
		int layer;
	};

}