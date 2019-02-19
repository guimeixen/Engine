#include "Object.h"

#include "Graphics\ResourcesLoader.h"
#include "Graphics\Model.h"
#include "Graphics\Renderer.h"
#include "Graphics\Animation\AnimatedModel.h"
#include "Graphics\Material.h"
#include "Graphics\ParticleSystem.h"

#include "Sound\SoundSource.h"

#include "Physics\Trigger.h"
#include "Physics\Collider.h"
#include "Physics\RigidBody.h"

#include "Program\Log.h"
#include "Program\Utils.h"

#include "include\glm\gtc\matrix_transform.hpp"
#include "include\glm\gtc\quaternion.hpp"
#include "include\glm\gtc\type_ptr.hpp"

#include <iostream>

namespace Engine
{
	Object::Object(Game *game, unsigned int id) : Transform(id)
	{
		type = ObjectType::DEFAULT_OBJECT;
		layer = Layer::DEFAULT;
		transformChanged = false;
	}

	void Object::SetLayer(int layer)
	{
		this->layer = layer;
	/*	if (collider)
		{
			game->GetPhysicsManager().ChangeLayer(collider, layer);
		}
		/*else if (rigidBody)
		{
			game->GetPhysicsManager().ChangeLayer(rigidBody, layer);
		}*/
	}

	void Object::SetLocalPosition(const glm::vec3 &position)
	{
		Transform::SetLocalPosition(position);
		transformChanged = true;
	}

	void Object::SetLocalRotation(const glm::quat &rot)
	{
		Transform::SetLocalRotation(rot);
		transformChanged = true;

		/*if (rigidBody)
		{
			glm::quat localRot = GetLocalRotation();

			glm::quat worldRot;
			if (GetParent())
			{
				// Not tested. 
				glm::quat worldRot = glm::inverse(GetParent()->GetLocalRotation()) * localRot;
				worldRot = glm::normalize(worldRot);
			}
			else
			{
				worldRot = localRot;
			}

			rigidBody->SetRotation(worldRot);
		}*/
	}

	void Object::SetLocalScale(const glm::vec3 &scale)
	{
		Transform::SetLocalScale(scale);
		transformChanged = true;
	}

	void Object::SetEnabled(bool enable)
	{
		isEnabled = enable;

		// Remove or re-add the light if the object was disabled or enabled
		if (!isEnabled)
		{
			/*if (light)
			{
				if (light->type == LightType::POINT)
					game->GetLightManager().RemovePointLight(static_cast<PointLight*>(light));
			}*/
			//if (collider)
			//	collider->SetEnabled(false);
		}
		else
		{
			/*if (light)
			{
				if (light->type == LightType::POINT)
					game->GetLightManager().EnablePointLight(static_cast<PointLight*>(light));
			}*/
			//if (collider)
			//	collider->SetEnabled(true);
		}

		const std::vector<Transform*> &children = GetChildren();
		for (size_t i = 0; i < children.size(); i++)
		{
			Object *obj = static_cast<Object*>(children[i]);
			obj->SetEnabled(enable);
		}
	}

	void Object::CopyTo(Object *dstObj)
	{
		if (!dstObj)
			return;

		Transform::CopyTo(dstObj);

		dstObj->layer = layer;
		dstObj->isEnabled = isEnabled;
	}

	void Object::Serialize(Serializer &s)
	{
		Transform::Serialize(s);
		s.Write(layer);
		s.Write(isEnabled);
	}

	void Object::Deserialize(Serializer &s)
	{
		Transform::Deserialize(s);
		transformChanged = true;

		s.Read(layer);
		s.Read(isEnabled);

		// Set the script in the trigger if we have one
		/*if (trigger)
		{
			if (script)
				trigger->SetScript(script);
		}*/
	}
}
