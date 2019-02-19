#include "SceneManager.h"

#include "Object.h"
#include "AI\AIObject.h"

#include "Graphics\Camera\Frustum.h"
#include "Graphics\Material.h"
#include "Graphics\ParticleSystem.h"
#include "Graphics\Camera\Camera.h"
#include "Graphics\Animation\AnimatedModel.h"
#include "Graphics\ResourcesLoader.h"

#include "Physics\RigidBody.h"

#include "Program\Utils.h"
#include "Program\StringID.h"
#include "Game\Game.h"

#include "include\glm\gtx\quaternion.hpp"
#include "include\glm\gtc\type_ptr.hpp"

#include <iostream>

namespace Engine
{
	void SceneManager::SavePrefab(Object *obj, const std::string &path)
	{
		if (!obj)
			return;

		Serializer s;
		s.OpenForWriting();
		SavePrefabRecursive(obj, s);
		s.Save(path);
		s.Close();
	}

	void SceneManager::SavePrefabRecursive(Transform *t, Serializer &s)
	{
		if (t->GetType() != ObjectType::TRANSFORM)		// Only serialize if it's not just a transform. Transforms only are only being used for bone attachments and don't need to be serialized
		{
			s.Write((unsigned int)t->GetType());
			t->Serialize(s);
		}
		const std::vector<Transform*> &children = t->GetChildren();

		for (size_t i = 0; i < children.size(); i++)
		{
			//Object *o = static_cast<Object*>(children[i]);
			SavePrefabRecursive(children[i], s);
		}
	}

	Object *SceneManager::LoadPrefab(const std::string &path)
	{
		Serializer s;
		s.OpenForReading(path);

		if (s.IsOpen())
		{
			std::vector<Object*> objs;

			while (!s.EndReached())
			{
				unsigned int type = 0;
				s.Read(type);					// TODO: Check if type is AIObject and load accordingly
				
				if (type == ObjectType::DEFAULT_OBJECT)
				{
					Object *obj = new Object(game, 9999);
					obj->Deserialize(s);

					objs.push_back(obj);

					objects.push_back(obj);
				}			
			}

			// Set children and parents
			for (size_t i = 0; i < objs.size(); i++)
			{
				Object *obj = objs[i];

				// Replace with entity
				/*AnimatedModel *am = obj->GetAnimatedModel();
				if (am)
					am->FillBoneAttachments(game);*/

				for (size_t j = 0; j < objs.size(); j++)
				{
					if (objs[j]->GetID() == obj->GetParentID())
					{
						obj->SetParent(objs[j], false);
					}

					// Replace with entity
					/*if (am)
					{
						const BoneAttachment *boneAttachments = am->GetBoneAttachments();
						for (unsigned int k = 0; k < am->GetBoneAttachmentsCount(); k++)
						{
							if (boneAttachments[k].objID == objs[j]->GetID())
							{
								//am->ReplaceBoneAttachmentObj(k, objs[j]);		// Cannot access protecte member		// replace with entity
							}
						}
					}*/
				}				
			}

			for (size_t i = 0; i < objs.size(); i++)
			{
				Object *obj = objs[i];

				// Replace with entity
				/*AnimatedModel *am = obj->GetAnimatedModel();
				if (am)
					am->UpdateBones(obj, 0.0f);			// Call the UpdateBones to set the bone transforms parents*/
			}

			s.Close();

			// Put the obj in front of the camera
			Object *firstObj = objs[0];
			firstObj->SetLocalPosition(game->GetMainCamera()->GetPosition() + game->GetMainCamera()->GetFront() * 4.0f);

			return firstObj;
		}

		return nullptr;
	}
}
