#pragma once

#include "Program\Serializer.h"

#include "include\glm\glm.hpp"
#include "include\glm\gtc\quaternion.hpp"

#include <vector>
#include <fstream>

namespace Engine
{
	enum ObjectType
	{
		DEFAULT_OBJECT,
		AI_OBJECT,
		TRANSFORM,		
	};

	class Transform
	{
	public:
		Transform(unsigned int id);
		virtual ~Transform();

		// Do not use the function. It is only used to reset the id's of the objects when we delete one
		void SetID(unsigned int id);
		unsigned int GetID() const { return id; }
		int GetParentID() const { return parentID; }

		void SetParent(Transform *newParent, bool calcLocalTransform = true);
		void RemoveChild(Transform *child);

		virtual void CopyTo(Transform *t);
		void DeleteChildren();

		Transform *GetParent() const { return parent; }
		Transform *GetRoot();
		const std::vector<Transform*> &GetChildren() const { return children; }

		virtual void SetLocalPosition(const glm::vec3 &position);
		glm::vec3 &GetLocalPosition() { return localPosition; }		// Don't add const otherwise when using get in lua scripts you will not be able to modify the value obtained

		virtual void SetLocalRotationEuler(const glm::vec3 &eulerRot);
		glm::vec3 &GetLocalRotationEuler() { return localRotEuler; }

		virtual void SetLocalRotation(const glm::quat &rot);
		const glm::quat &GetLocalRotation() const { return localRotation; }

		virtual void SetLocalScale(const glm::vec3 &scale);
		glm::vec3 &GetLocalScale() { return localScale; }

		virtual void SetWorldPosition(const glm::vec3 &position);
		glm::vec3 GetWorldPosition() { if (isDirty) { GetLocalToWorldTransform(); } return worldPosition; }

		virtual void SetWorldRotEuler(const glm::vec3 &rotEuler);
		glm::vec3 GetWorldRotEuler() { return worldRotEuler; }

		virtual void SetWorldScale(const glm::vec3 &scale);
		glm::vec3 GetWorldScale() { return worldScale; }

		glm::vec3 GetForward();
		glm::vec3 GetRight();
		glm::vec3 GetUp();

		const glm::mat4 &GetLocalToParentTransform();
		const glm::mat4 &GetInvLocalToWorldTransform();
		const glm::mat4 &GetLocalToWorldTransform();

		void SetLocalToWorldTransform(const glm::mat4 &t);
		void SetLocalToParentTransform(const glm::mat4 &t);

		ObjectType GetType() const { return type; }

		virtual void Serialize(Serializer &s);
		virtual void Deserialize(Serializer &s);

	private:
		void SetDirty();
		void UpdateWorldTransform();

	protected:
		bool isDirty;
		ObjectType type;

	private:
		unsigned int id;
		int parentID;		// Just used to know which parent each transform has when load the from file		

		Transform *parent;
		std::vector<Transform*> children;

		glm::mat4 localToWorld;
		glm::mat4 invLocalToWorld;
		glm::mat4 localToParent;

		bool isInvDirty;

		glm::vec3 localPosition;
		glm::quat localRotation;
		glm::vec3 localRotEuler;
		glm::vec3 localScale;

		glm::vec3 worldPosition;
		glm::quat worldRotation;
		glm::vec3 worldRotEuler;
		glm::vec3 worldScale;
	};
}
