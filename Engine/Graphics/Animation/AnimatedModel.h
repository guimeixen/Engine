#pragma once

#include "Game/ComponentManagers/TransformManager.h"
#include "Graphics/Model.h"
#include "AnimationController.h"

#include "Program/Utils.h"

#include "include/glm/gtx/quaternion.hpp"
#include "include/assimp/Importer.hpp"

#include <map>

#define MAX_BONES_PER_VERTEX 4
#define MAX_BONES_CONNECTIONS 4

namespace Engine
{
	struct Keyframe
	{
		glm::vec3 position;
		glm::quat rotation;
		glm::vec3 scale;
		float time;
	};

	typedef std::vector<Keyframe> KeyFrameList;

	struct Bone
	{
		std::string name;
		glm::mat4 transformation;			// Bone transformation relative to parent
		std::vector<Bone*> children;
		Bone *parent;

		~Bone()
		{
			for (size_t i = 0; i < children.size(); i++)
			{
				if (children[i])
					delete children[i];
			}
		}
	};

	struct BoneAttachment
	{
		Bone *bone;
		Entity boneTransformEntity;
		Entity attachedEntity;
	};

	struct Animation
	{
		std::string name;
		std::string path;
		float duration;
		float ticksPerSecond;
		bool loadedSeparately;
		bool useRootMotion;
		std::map<std::string, KeyFrameList> bonesKeyframesList;	// Maps a bone name to a list of all those bone's keyframes for this animation

		unsigned int refCount;

		void Store(aiAnimation *anim)
		{
			name = std::string(anim->mName.data);
			if (name.empty())
				name = "Animation";

			duration = static_cast<float>(anim->mDuration);
			ticksPerSecond = (float)(anim->mTicksPerSecond != 0.0 ? anim->mTicksPerSecond : 25.0f);

			for (unsigned int i = 0; i < anim->mNumChannels; i++)		// Num channels are the number of bones in this animation
			{
				aiNodeAnim *bone = anim->mChannels[i];
				if (bone->mNumPositionKeys <= 0)
					continue;

				KeyFrameList list = {};
				list.resize(bone->mNumPositionKeys);

				for (unsigned int j = 0; j < bone->mNumPositionKeys; j++)
				{
					const aiVector3D &posValue = bone->mPositionKeys[j].mValue;
					const aiQuaternion &rotValue = bone->mRotationKeys[j].mValue;
					//const aiVector3D &scaleValue = bone->mScalingKeys[j].mValue;

					list[j].position = glm::vec3(posValue.x, posValue.y, posValue.z);
					list[j].rotation = glm::quat(rotValue.w, rotValue.x, rotValue.y, rotValue.z);
					list[j].scale = glm::vec3(1.0f);
					list[j].time = static_cast<float>(bone->mPositionKeys[j].mTime);
				}

				bonesKeyframesList.insert({ std::string(bone->mNodeName.data), list });
			}

			//animations.push_back(a);
		}

		void AddReference() { refCount++; }
		void RemoveReference() { if (refCount > 1) { --refCount; } else { delete this; } }
	};

	class AnimatedModel : public Model
	{
	public:
		AnimatedModel();
		AnimatedModel(Game *game, const std::string &path);
		AnimatedModel(Game *game, const std::string &path, const std::vector<std::string> &matNames);
		~AnimatedModel();

		void AddAnimation(Animation *anim);
		void SetAnimationController(const std::string &controllerPath, ModelManager *modelManager);
		void RemoveAnimationController() { if (animController) { delete animController; animController = nullptr; } }

		unsigned short AddBoneAttachment(Game *game, Bone *bone, Entity attachedEntity);
		unsigned short AddBoneAttachment(Game *game, const std::string &boneName, Entity attachedEntity);
		void SwitchBoneAttachmentEntity(TransformManager &transformManager, unsigned short boneAttachID, Entity newAttachedEntity);
		void RemoveBoneAttachment(EntityManager &entityManager, const BoneAttachment *boneAttach);
		void RemoveBoneAttachments(TransformManager &transformManager);
		void FillBoneAttachments(Game *game);
		const BoneAttachment *GetBoneAttachments() const { return boneAttachments; }
		unsigned short GetBoneAttachmentsCount() const { return curBoneAttachments; }
		void Dispose(Game *game);

		void SetDirty() { isDirty = true; }
		bool IsDirty() const { return isDirty; }

		void UpdateController();
		void UpdateBones(TransformManager &transformManager, Entity self, float deltaTime);
		void PlayAnimationStr(const std::string &name);
		void PlayAnimation(unsigned int index);
		void PauseAnimation(bool pause);
		void StopAnimation(unsigned int index);
		void TransitionTo(unsigned int index, float time, bool loop);
		void TransitionTo(Animation *anim, float time, bool loop);
		void RevertTransition();
		bool IsTransitionFinished() const { return isTransitionFinished; }
		bool IsTransitioning() const { return shouldTransition; }
		bool IsAnimationFinished() const { return isAnimationFinished; }
		bool IsReverting() const { return shouldRevert; }
		unsigned short GetCurrentKeyFrameIndex() const { return curKeyFrame; }

		Bone *GetRootBone() const { return rootBone; }
		const std::vector<glm::mat4> &GetBoneTransforms() const { return boneTransforms; }
		const std::vector<Animation*> &GetAnimations() const { return animations; }
		Animation *GetCurrentAnimation() const { return animations[currentAnimation]; }
		AnimationController *GetAnimationController() { return animController; }

		void Serialize(Serializer &s);
		void Deserialize(Serializer &s, Game *game, bool reload = false);

	protected:
		void ReplaceBoneAttachmentEntity(unsigned int boneAttachID, Entity newAttachedEntity);

	private:
		void LoadModelFile(Game *game, Renderer *renderer, const std::vector<std::string> &matNames, ScriptManager &scriptManager, bool loadVertexColors);
		Mesh ProcessMesh(Renderer *renderer, const aiMesh *aimesh, const aiScene *aiscene, bool loadVertexColors);

		bool FindBone(aiAnimation *anim, const std::string &nodeName);
		void BuildBoneTree(aiNode *node, Bone *parent);
		unsigned int FindKeyFrame(float animTime, const KeyFrameList &keyFrameList);
		void Interpolate(float animTime, glm::vec3 &position, glm::quat &rot, glm::vec3 &scale, const KeyFrameList &keyFrameList);
		void UpdateBoneTree(float curAnimTime, float nextAnimTime, const Bone *bone, const glm::mat4 &parentTransform);

		void GetBoneTransform(const Bone *bone, glm::mat4 &transform);
		void FindBoneByName(Bone *startingBone, const std::string &boneName, Bone **outBone);

	private:
		Bone *rootBone;
		AnimationController *animController;
		std::vector<glm::mat4> boneTransforms;
		std::vector<glm::mat4> boneOffsetMatrices;
		std::vector<Animation*> animations;

		BoneAttachment boneAttachments[MAX_BONES_CONNECTIONS];
		unsigned short curBoneAttachments = 0;

		bool isDirty = true;

		unsigned int currentAnimation = 0;
		unsigned int nextAnim = 0;
		float transitionTime = 1.0f;

		bool shouldTransition = false;
		bool shouldRevert = false;
		float blendFactor = 0.0f;
		float currentTime = 0.0f;
		bool isTransitionFinished = false;

		bool isAnimationFinished = false;

		float animTime = 0.0f;
		float elapsedTime = 0.0f;

		bool loopCurrent = true;
		bool loopNext = true;

		bool isPaused = false;

		std::map<std::string, unsigned int> boneMap;
		unsigned int numBones = 0;

		unsigned short curKeyFrame = 0;

		Assimp::Importer importer;
		glm::mat4 globalInvTransform;
	};
}
