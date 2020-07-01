#include "AnimationController.h"

#include "Program/StringID.h"
#include "AnimatedModel.h"
#include "Graphics/ResourcesLoader.h"
#include "Game/ComponentManagers/ModelManager.h"

#include <iostream>

namespace Engine
{
	AnimationController::AnimationController(const std::string &path)
	{
		this->path = path;
		// Add entry node
		AnimNode entry;
		entry.SetName("Entry");
		animNodes.push_back(entry);
		curNodeID = 0;
		transitioningToPrevious = false;
	}

	void AnimationController::Update(AnimatedModel *animModel)
	{
		if (animModel->IsTransitionFinished() && animModel->IsAnimationFinished() && animNodes[curNodeID].IsLooping() == false)
		{
			// Transition back to the previous node
			if (animNodes[curNodeID].GetLinks().size() > 0)
			{
				AnimNode &previousNode = animNodes[previousNodeID];
				Animation *anim = previousNode.GetAnim();
				unsigned int temp = curNodeID;
				curNodeID = previousNodeID;
				previousNodeID = temp;

				animModel->TransitionTo(anim, previousNode.GetLinks()[previousLinkID].GetTransitionTime(), previousNode.IsLooping());
				transitioningToPrevious = true;

				//std::cout << "Transitioning to previous node: "  << previousNodeID << ", from current node: "  << curNodeID << '\n';
			}
		}

		if (transitioningToPrevious && animModel->IsTransitionFinished())
			transitioningToPrevious = false;

		if (!transitioningToPrevious)
		{
			// Check if condition if valid on any of the current node links
			const std::vector<Link> &links = animNodes[curNodeID].GetLinks();
			for (size_t i = 0; i < links.size(); i++)
			{
				const Link &link = links[i];
				const std::vector<Condition> &conditions = link.GetConditions();

				unsigned int numConditionsPassed = 0;

				for (size_t j = 0; j < conditions.size(); j++)
				{
					const Condition &c = conditions[j];
					const ParameterDesc &pd = parametersDesc[c.paramID];

					if (pd.type == ParamType::INT)
					{
						if (c.condition == TransitionCondition::GREATER)
						{
							if (pd.param.intVal > c.param.intVal)
								numConditionsPassed++;
						}
						else if (c.condition == TransitionCondition::LESS)
						{
							if (pd.param.intVal < c.param.intVal)
								numConditionsPassed++;
						}
						else if (c.condition == TransitionCondition::EQUAL)
						{
							if (pd.param.intVal == c.param.intVal)
								numConditionsPassed++;
						}
						else if (c.condition == TransitionCondition::NOT_EQUAL)
						{
							if (pd.param.intVal != c.param.intVal)
								numConditionsPassed++;
						}
					}
					// FLOAT
					else if (pd.type == ParamType::FLOAT)
					{
						if (c.condition == TransitionCondition::GREATER)
						{
							if (pd.param.floatVal > c.param.floatVal)
								numConditionsPassed++;
						}
						else if (c.condition == TransitionCondition::LESS)
						{
							if (pd.param.floatVal < c.param.floatVal)
								numConditionsPassed++;
						}
					}
					// BOOL 
					else if (pd.type == ParamType::BOOL)
					{
						if (c.condition == TransitionCondition::EQUAL)
						{
							if (pd.param.boolVal == c.param.boolVal)
								numConditionsPassed++;
						}
						else if (c.condition == TransitionCondition::NOT_EQUAL)		// Use not equal as false
						{
							if (pd.param.boolVal == c.param.boolVal)
								numConditionsPassed++;
						}
					}
				}

				// If all conditions passed then make the transition
				if (numConditionsPassed == conditions.size())
				{
					unsigned int nextNodeID = link.GetToNodeID();
					Animation *anim = animNodes[nextNodeID].GetAnim();

					//std::cout << anim->path << '\n';

					if (animModel->IsTransitioning() /*&& anim == animModel->GetCurrentAnimation()*/)
					{
						animModel->RevertTransition();
						//std::cout << "reverting\n";
					}
					else if(!animModel->IsReverting())
					{
						previousNodeID = curNodeID;
						previousLinkID = i;

						curNodeID = nextNodeID;

						// Only update visually if it's not the same animation
						if (anim != animModel->GetCurrentAnimation())
							animModel->TransitionTo(anim, link.GetTransitionTime(), animNodes[curNodeID].IsLooping());

						//std::cout << "transitioning to: " << curNodeID << '\n';
						//std::cout << anim->path << '\n';
					}
					
					break;			// We don't check anymore conditions of this link because we found one that passes the condition
				}
			}
		}
	}

	void AnimationController::SetInt(const std::string &name, int value)
	{
		for (size_t i = 0; i < parametersDesc.size(); i++)
		{
			if (parametersDesc[i].name == name)
			{
				parametersDesc[i].param.intVal = value;
			}
		}
	}

	void AnimationController::SetFloat(const std::string &name, float value)
	{
		for (size_t i = 0; i < parametersDesc.size(); i++)
		{
			if (parametersDesc[i].name == name)
			{
				parametersDesc[i].param.floatVal = value;
			}
		}
	}

	void AnimationController::SetBool(const std::string &name, bool value)
	{
		for (size_t i = 0; i < parametersDesc.size(); i++)
		{
			if (parametersDesc[i].name == name)
			{
				parametersDesc[i].param.boolVal = value;
			}
		}
	}

	void AnimationController::Serialize(Serializer &s) const
	{
		s.Write(static_cast<unsigned int>(animNodes.size()));
		s.Write(static_cast<unsigned int>(parametersDesc.size()));

		for (size_t i = 0; i < animNodes.size(); i++)
		{
			animNodes[i].Serialize(s);
		}

		for (size_t i = 0; i < parametersDesc.size(); i++)
		{
			parametersDesc[i].Serialize(s);
		}
	}

	void AnimationController::Deserialize(Serializer &s, ModelManager *modelManager)
	{
		unsigned int numAnimNodes = 0;
		s.Read(numAnimNodes);
		animNodes.resize(numAnimNodes);

		unsigned int numParameters = 0;
		s.Read(numParameters);
		parametersDesc.resize(numParameters);

		for (size_t i = 0; i < animNodes.size(); i++)
		{
			animNodes[i].Deserialize(s, modelManager);
		}

		for (size_t i = 0; i < parametersDesc.size(); i++)
		{
			parametersDesc[i].Deserialize(s);
		}
	}

	AnimNode::AnimNode()
	{
		animID = 0;
		anim = nullptr;
		loop = true;
	}

	void AnimNode::SetAnimationID(ModelManager *modelManager, unsigned int id)
	{
		animID = id;

		// Grab animation from the loaded animations using the animID
		const std::map<unsigned int, Animation*> &animations = modelManager->GetAnimations();
		anim = animations.at(animID);
	}

	void AnimNode::SetAnimation(Animation *anim)
	{
		this->anim = anim;
	}

	void AnimNode::Serialize(Serializer &s) const
	{
		s.Write(animID);
		s.Write(loop);
		s.Write(name);
		s.Write(static_cast<unsigned int>(links.size()));

		for (size_t i = 0; i < links.size(); i++)
		{
			links[i].Serialize(s);
		}
	}

	void AnimNode::Deserialize(Serializer &s, ModelManager *modelManager)
	{
		s.Read(animID);

		if (animID != 0)
		{
			// Grab animation from the loaded animations using the animID
			const std::map<unsigned int, Animation*> &animations = modelManager->GetAnimations();
			if (animations.find(animID) != animations.end())
				anim = animations.at(animID);
		}

		s.Read(loop);
		s.Read(name);

		unsigned int numLinks = 0;
		s.Read(numLinks);
		links.resize(numLinks);

		for (size_t i = 0; i < links.size(); i++)
		{
			links[i].Deserialize(s);
		}
	}

	Link::Link()
	{
		transitionTime = 1.0f;
		toNodeID = -1;
	}

	void Link::Serialize(Serializer &s) const
	{
		s.Write(toNodeID);
		s.Write(transitionTime);
		s.Write(static_cast<unsigned int>(conditions.size()));

		for (size_t i = 0; i < conditions.size(); i++)
		{
			conditions[i].Serialize(s);
		}
	}

	void Link::Deserialize(Serializer &s)
	{
		s.Read(toNodeID);
		s.Read(transitionTime);
		
		unsigned int numConditions = 0;
		s.Read(numConditions);
		conditions.resize(numConditions);

		for (size_t i = 0; i < conditions.size(); i++)
		{
			conditions[i].Deserialize(s);
		}
	}

	void Condition::Serialize(Serializer &s) const
	{
		s.Write(condition);
		s.Write(paramID);
		s.Write(param.intVal);
		s.Write(param.floatVal);
		s.Write(param.boolVal);
	}

	void Condition::Deserialize(Serializer &s)
	{
		int cond = 0;
		s.Read(cond);
		condition = (TransitionCondition)cond;
		s.Read(paramID);
		s.Read(param.intVal);
		s.Read(param.floatVal);
		s.Read(param.boolVal);
	}

	void ParameterDesc::Serialize(Serializer &s) const
	{
		s.Write(static_cast<int>(type));
		s.Write(name);
		// There's no need to serialize the param value because it will probably be modified and it doesn't matter if it starts initialized at 0
	}

	void ParameterDesc::Deserialize(Serializer &s)
	{
		int t = 0;
		s.Read(t);
		type = (ParamType)t;

		s.Read(name);
	}
}
