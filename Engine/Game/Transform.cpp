#include "Transform.h"

#include "Program\Utils.h"

#include "include\glm\gtc\matrix_transform.hpp"
#include "include\glm\gtx\euler_angles.hpp"

#include <iostream>

namespace Engine
{
	Transform::Transform(unsigned int id)
	{
		this->id = id;
		parentID = -1;
		parent = nullptr;
		type = ObjectType::TRANSFORM;

		localToParent = glm::mat4(1.0f);
		localToWorld = glm::mat4(1.0f);

		localPosition = glm::vec3(0.0f);
		localRotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
		localRotEuler = glm::vec3(0.0f);
		localScale = glm::vec3(1.0f);

		worldPosition = glm::vec3(0.0f);
		worldRotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
		worldRotEuler = glm::vec3(0.0f);
		worldScale = glm::vec3(1.0f);

		isDirty = true;
		isInvDirty = true;
	}

	Transform::~Transform()
	{
	}

	void Transform::SetID(unsigned int id)
	{
		this->id = id;
	}

	void Transform::SetParent(Transform *newParent, bool calcLocalTransform)
	{
		if (newParent)
		{
			if (parent)
				parent->RemoveChild(this);

			parent = newParent;
			parent->children.push_back(this);

			if (calcLocalTransform)				// Used so when we add a child, it's transform gets updated so it has the position, etc relative to the new parent
			{
				// Update the child transform relative to the parent
				glm::mat4 t = parent->GetInvLocalToWorldTransform() * GetLocalToWorldTransform();
				SetLocalPosition(t[3]);
				SetLocalScale(glm::vec3(glm::length(t[0]), glm::length(t[1]), glm::length(t[2])));

				glm::quat m = glm::inverse(parent->localRotation) * localRotation;
				m = glm::normalize(m);
				SetLocalRotation(m);
			}
			// Changing parent affects the transform so we need to recalculate it
			SetDirty();
		}
	}

	void Transform::RemoveChild(Transform *child)
	{
		for (auto it = children.begin(); it != children.end(); it++)
		{
			if (*it == child)
			{
				children.erase(it);

				// When removing the child, she goes back to world space so set the new local position and rotation (which are now in world space)
				glm::mat4 t = child->GetLocalToWorldTransform();

				child->SetLocalPosition(t[3]);

				child->SetLocalScale(glm::vec3(glm::length(t[0]), glm::length(t[1]), glm::length(t[2])));

				// We need a rotation matrix to convert to quaternion
				// There's no need to remove the translation because it's going to be casted to a 3x3 matrix so it's going to be dropped
				// Set scale to 1
				t[0] = glm::normalize(t[0]);
				t[1] = glm::normalize(t[1]);
				t[2] = glm::normalize(t[2]);
				glm::quat q = glm::quat_cast(t);
				q = glm::normalize(q);

				child->SetLocalRotation(q);

				child->parent = nullptr;
				break;
			}
		}
	}

	void Transform::CopyTo(Transform *t)
	{
		if (!t)
			return;

		//t->id = 0;		// !!!!!! Don't set the ID because it was set when  t  was created
		//t->parentID = parentID;
		t->localPosition = localPosition;
		t->localRotation = localRotation;
		t->localRotEuler = localRotEuler;
		t->localScale = localScale;

		t->worldPosition = worldPosition;
		t->worldRotation = worldRotation;
		t->worldRotEuler = worldRotEuler;
		t->worldScale = worldScale;

		t->isDirty = true;
		t->isInvDirty = true;

		//if (parent)
		//	t->parent = parent;
	}

	void Transform::DeleteChildren()
	{
		for (size_t i = 0; i < children.size(); i++)
		{
			children[i]->DeleteChildren();
			delete children[i];
		}
	}

	Transform *Transform::GetRoot()
	{
		if (parent != nullptr)
		{
			return parent->GetRoot();
		}
		return this;
	}

	void Transform::SetLocalPosition(const glm::vec3 &position)
	{
		localPosition = position;
		SetDirty();
	}

	void Transform::SetLocalRotationEuler(const glm::vec3 &eulerRot)
	{
		glm::quat xRot = glm::angleAxis(glm::radians(eulerRot.x), glm::vec3(1.0f, 0.0f, 0.0f));
		glm::quat yRot = glm::angleAxis(glm::radians(eulerRot.y), glm::vec3(0.0f, 1.0f, 0.0f));
		glm::quat zRot = glm::angleAxis(glm::radians(eulerRot.z), glm::vec3(0.0f, 0.0f, 1.0f));

		//localRotation = glm::quat(glm::vec3(glm::radians(eulerRot.x), glm::radians(eulerRot.y), glm::radians(eulerRot.z)));
		localRotation = yRot * xRot * zRot;
		localRotation = glm::normalize(localRotation);
		localRotEuler = eulerRot;
		SetDirty();
	}

	void Transform::SetLocalRotation(const glm::quat &rot)
	{
		localRotation = rot;

		glm::dvec3 v;
		double test = localRotation.x * localRotation.y + localRotation.z * localRotation.w;
		if (test > 0.499)			 // singularity at north pole
		{
			v.x = 2.0 * atan2((double)localRotation.x, (double)localRotation.w);
			v.y = 3.14159f / 2.0;
			v.z = 0.0;
			return;
		}
		if (test < -0.499)		// singularity at south pole
		{ 
			v.x = -2.0 * atan2((double)localRotation.x, (double)localRotation.w);
			v.y= -3.14159f / 2.0;
			v.z = 0.0;
			return;
		}
		double sqx = localRotation.x * localRotation.x;
		double sqy = localRotation.y * localRotation.y;
		double sqz = localRotation.z * localRotation.z;
		v.x = atan2(2.0 * localRotation.y * localRotation.w - 2.0 * localRotation.x * localRotation.z, 1.0 - 2.0 * sqy - 2.0 * sqz);
		v.y = asin(2.0 * test);
		v.z = atan2(2.0 * localRotation.x * localRotation.w - 2.0 * localRotation.y * localRotation.z, 1.0 - 2.0 * sqx - 2.0 * sqz);

		localRotEuler = glm::degrees(glm::vec3(v.z, v.x, v.y));

		SetDirty();
	}

	void Transform::SetLocalScale(const glm::vec3 &scale)
	{
		localScale = scale;
		SetDirty();
	}

	void Transform::SetWorldPosition(const glm::vec3 &position)
	{
		worldPosition = position;
		UpdateWorldTransform();
	}

	void Transform::SetWorldRotEuler(const glm::vec3 &rotEuler)
	{
		worldRotation = glm::quat(glm::vec3(glm::radians(rotEuler.x), glm::radians(rotEuler.y), glm::radians(rotEuler.z)));
		worldRotation = glm::normalize(worldRotation);
		worldRotEuler = rotEuler;
		UpdateWorldTransform();
	}

	void Transform::SetWorldScale(const glm::vec3 &scale)
	{
		worldScale = scale;
		UpdateWorldTransform();
	}

	void Transform::UpdateWorldTransform()
	{
		//glm::mat4 t = glm::mat4_cast(worldRotation);
		glm::mat4 t = glm::eulerAngleYXZ(glm::radians(worldRotEuler.y), glm::radians(worldRotEuler.x), glm::radians(worldRotEuler.z));
		t[3] = glm::vec4(worldPosition, 1.0f);
		t = glm::scale(t, worldScale);
		SetLocalToWorldTransform(t);
	}

	glm::vec3 Transform::GetForward()
	{
		return glm::normalize(GetLocalToWorldTransform()[2]);
	}

	glm::vec3 Transform::GetRight()
	{
		return glm::normalize(GetLocalToWorldTransform()[0]);
	}

	glm::vec3 Transform::GetUp()
	{
		return glm::normalize(GetLocalToWorldTransform()[1]);
	}

	const glm::mat4 &Transform::GetLocalToParentTransform()
	{
		localToParent = glm::mat4_cast(localRotation);
		//localToParent = glm::eulerAngleYXZ(glm::radians(localRotEuler.y), glm::radians(localRotEuler.x), glm::radians(localRotEuler.z));
		localToParent[3] = glm::vec4(localPosition, 1.0f);
		localToParent = glm::scale(localToParent, localScale);

		return localToParent;
	}

	const glm::mat4 &Transform::GetInvLocalToWorldTransform()
	{
		if (isInvDirty)
		{
			invLocalToWorld = glm::inverse(GetLocalToWorldTransform());
			isInvDirty = false;
		}

		return invLocalToWorld;
	}

	const glm::mat4 &Transform::GetLocalToWorldTransform()
	{
		if (isDirty)
		{
			if (!parent)
			{
				localToWorld = GetLocalToParentTransform();
				worldPosition = localPosition;
				worldRotation = localRotation;
				worldRotEuler = localRotEuler;
				worldScale = localScale;
			}
			else
			{
				localToWorld = parent->GetLocalToWorldTransform() * GetLocalToParentTransform();
				worldPosition = localToWorld[3];
				worldRotation = glm::quat_cast(localToWorld);
				worldRotation = glm::normalize(worldRotation);
				worldRotEuler = glm::degrees(glm::eulerAngles(worldRotation));
			}
			isDirty = false;
		}

		return localToWorld;
	}

	void Transform::SetLocalToWorldTransform(const glm::mat4 &t)
	{
		if (parent)
		{
			worldPosition = t[3];
			worldRotation = glm::quat_cast(t);
			worldRotation = glm::normalize(worldRotation);
			worldRotEuler = glm::degrees(glm::eulerAngles(worldRotation));
			// TODO: Set scale
			glm::mat4 m = parent->GetInvLocalToWorldTransform() * t;

			localPosition = m[3];

			localRotation = glm::quat_cast(m);
			localRotation = glm::normalize(localRotation);
			localRotEuler = glm::degrees(glm::eulerAngles(localRotation));

			localToWorld = parent->GetLocalToWorldTransform() * GetLocalToParentTransform();
		}
		else
		{
			localPosition = t[3];
			// TODO: Set scale
			localRotation = glm::quat_cast(t);
			localRotation = glm::normalize(localRotation);
			localRotEuler = glm::degrees(glm::eulerAngles(localRotation));

			worldPosition = localPosition;
			worldRotation = localRotation;
			worldRotEuler = localRotEuler;

			localToWorld = GetLocalToParentTransform();
		}
		// Only set the children dirty because the parent's transform has already been calculated
		for (size_t i = 0; i < children.size(); i++)
		{
			children[i]->SetDirty();
		}
	}

	void Transform::SetLocalToParentTransform(const glm::mat4 &t)
	{
		localPosition = t[3];
		// TODO: Set scale
		localRotation = glm::quat_cast(t);
		localRotation = glm::normalize(localRotation);
		localRotEuler = glm::degrees(glm::eulerAngles(localRotation));

		//localToParent = GetLocalToParentTransform();

		// Only set the children dirty because the parent's transform has already been calculated
		/*for (size_t i = 0; i < children.size(); i++)
		{
			children[i]->SetDirty();
		}*/

		SetDirty();
	}

	void Transform::Serialize(Serializer &s)
	{
		s.Write(id);
		if (parent)
			s.Write((int)parent->GetID());
		else
			s.Write(-1);
		s.Write(localPosition);
		//s.Write(localRotEuler);		// We no longer save the euler rotation because it was causing wrong rotations when loading after saving a project in which we just set an object as child of another
		s.Write(localRotation);
		s.Write(localScale);
	}

	void Transform::Deserialize(Serializer &s)
	{
		s.Read(id);
		s.Read(parentID);
		s.Read(localPosition);
		//s.Read(localRotEuler);
		s.Read(localRotation);
		s.Read(localScale);

		// We need to set the rotation euler so we update the transform otherwise the rotation will not be calculated properly
		//SetDirty();					// Also no need to call SetDirty here because SetLocalRotationEuler already does it 
		//SetLocalRotationEuler(localRotEuler);
		SetLocalRotation(localRotation);
		//SetDirty();
	}

	void Transform::SetDirty()
	{
		isDirty = true;
		isInvDirty = true;

		for (size_t i = 0; i < children.size(); i++)
		{
			children[i]->SetDirty();
		}
	}
}
