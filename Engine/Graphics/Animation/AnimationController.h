#pragma once

#include "Program/Serializer.h"

#include <string>
#include <vector>
#include <map>

namespace Engine
{
	class AnimatedModel;
	struct Animation;
	class ModelManager;

	enum class ParamType
	{
		INT,
		FLOAT,
		BOOL
	};

	enum TransitionCondition
	{
		GREATER,
		LESS,
		EQUAL,
		NOT_EQUAL,
	};

	union Parameter
	{
		int intVal;
		float floatVal;
		bool boolVal;
	};

	struct ParameterDesc
	{
		Parameter param;
		ParamType type;
		std::string name;

		void Serialize(Serializer &s) const;
		void Deserialize(Serializer &s);
	};

	struct Condition
	{
		TransitionCondition condition;
		Parameter param;
		unsigned int paramID;

		void Serialize(Serializer &s) const;
		void Deserialize(Serializer &s);
	};

	class Link
	{
	public:
		Link();

		void SetToNodeID(int id) { toNodeID = id; }
		void SetTransitionTime(float time) { transitionTime = time; }

		void AddParamID(unsigned int id) { Condition c = {}; c.paramID = id; conditions.push_back(c); }
		void SetCondition(unsigned int conditionID, TransitionCondition tc) { if (conditionID < conditions.size()) { conditions[conditionID].condition = tc; } }
		void SetConditionInt(unsigned int conditionID, int value) { if (conditionID < conditions.size()) { conditions[conditionID].param.intVal = value; } }
		void SetConditionFloat(unsigned int conditionID, float value) { if (conditionID < conditions.size()) { conditions[conditionID].param.floatVal = value; } }
		void SetConditionBool(unsigned int conditionID, bool value) { if (conditionID < conditions.size()) { conditions[conditionID].param.boolVal = value; } }

		float GetTransitionTime() const { return transitionTime; }
		int GetToNodeID() const { return toNodeID; }
		const std::vector<Condition> &GetConditions() const { return conditions; }

		void Serialize(Serializer &s) const;
		void Deserialize(Serializer &s);

	private:
		int toNodeID;
		float transitionTime;
		std::vector<Condition> conditions;
	};

	class AnimNode
	{
	public:
		AnimNode();

		void AddLink(const Link &link) { links.push_back(link); }
		void RemoveLink(unsigned int index) { if (index < links.size()) { links.erase(links.begin() + index); } }

		void SetAnimationID(ModelManager *modelManager, unsigned int id);
		void SetAnimation(Animation *anim);
		void SetName(const std::string &name) { this->name = name; }
		void SetLooping(bool loop) { this->loop = loop; }

		const std::string &GetName() const { return name; }
		std::vector<Link> &GetLinks() { return links; }
		bool IsLooping() const { return loop; }
		unsigned int GetAnimID() const { return animID; }
		Animation *GetAnim() const { return anim; }

		void Serialize(Serializer &s) const;
		void Deserialize(Serializer &s, ModelManager *modelManager);

	private:
		unsigned int animID;
		Animation *anim;
		std::string name;
		//float playbackSpeed;
		std::vector<Link> links;
		bool loop;
	};

	class AnimationController
	{
	public:
		AnimationController(const std::string &path);

		void Update(AnimatedModel *animModel);

		void SetInt(const std::string &name, int value);
		void SetFloat(const std::string &name, float value);
		void SetBool(const std::string &name, bool value);		

		void AddNode(const AnimNode &node) { animNodes.push_back(node); }

		std::vector<AnimNode> &GetAnimNodes()  { return animNodes; }
		std::vector<ParameterDesc> &GetParameters() { return parametersDesc; }
		unsigned int GetCurrentNodeID() const { return curNodeID; }
		const std::string &GetPath() const { return path; }

		bool IsTransitioningToPrevious() const { return transitioningToPrevious; }

		void Serialize(Serializer &s) const;
		void Deserialize(Serializer &s, ModelManager *modelManager);

	private:
		std::vector<AnimNode> animNodes;
		std::vector<ParameterDesc> parametersDesc;
		unsigned int curNodeID;
		unsigned int previousNodeID;
		unsigned int previousLinkID;
		std::string path;
		bool transitioningToPrevious;
	};
}
