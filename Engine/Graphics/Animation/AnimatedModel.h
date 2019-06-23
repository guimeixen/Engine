#pragma once

#include "Game/ComponentManagers/TransformManager.h"
#include "Graphics/Model.h"
#include "AnimationController.h"
#include "AnimationController.h"

#include "Program/Utils.h"

#include "include/glm/gtx/quaternion.hpp"

#include <map>

#define MAX_BONES_PER_VERTEX 4
#define MAX_BONES_CONNECTIONS 4

struct aiAnimation;
struct aiNode;

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
		//bool loadedSeparately;
		//bool useRootMotion;
		std::map<std::string, KeyFrameList> bonesKeyframesList;	// Maps a bone name to a list of all those bone's keyframes for this animation

		unsigned int refCount;

		void AddReference() { refCount++; }
		void RemoveReference() { if (refCount > 1) { --refCount; } else { delete this; } }
	};

	class AnimatedModel : public Model
	{
	public:
		AnimatedModel();
		AnimatedModel(Renderer *renderer, ScriptManager &scriptManager, const std::string &path, const std::vector<std::string> &matNames);
		~AnimatedModel();

		void AddAnimation(Animation *anim);
		void SetAnimationController(FileManager *fileManager, const std::string &controllerPath, ModelManager *modelManager);
		void RemoveAnimationController() { if (animController) { delete animController; animController = nullptr; } }

		void AddBoneOffsetMatrix(const glm::mat4 &mat) { boneOffsetMatrices.push_back(mat); }
		void AddBoneTransform(const glm::mat4 &mat) { boneTransforms.push_back(mat); }
		void SetGlobalInvTransform(const glm::mat4 &mat) { globalInvTransform = mat; }
		std::map<std::string, unsigned int> &GetBoneMap() { return boneMap; }
		std::vector<glm::mat4> &GetBoneTransforms() { return boneTransforms; }
		std::vector<glm::mat4> &GetBoneOffsetMatrices() { return boneOffsetMatrices; }

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
		//bool FindBone(aiAnimation *anim, const std::string &nodeName);

		void LoadModel(Renderer *renderer, ScriptManager &scriptManager, const std::vector<std::string> &matNames);
		void ReadBoneTree(Serializer &s, Bone *bone);

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

		unsigned short curKeyFrame = 0;

		glm::mat4 globalInvTransform;
	};
}
