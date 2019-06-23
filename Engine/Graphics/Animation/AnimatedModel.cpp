#include "AnimatedModel.h"

#include "Graphics/ResourcesLoader.h"
#include "Graphics/VertexArray.h"
#include "Graphics/Buffers.h"
#include "Graphics/Renderer.h"
#include "Program/Log.h"
#include "Game/Game.h"
#include "Program/Input.h"
#include "Graphics/Material.h"

#include "include/glm/gtc/matrix_transform.hpp"
#include "include/glm/glm.hpp"
#include "include/glm/gtc/type_ptr.hpp"

#include <iostream>
#include <cmath>

namespace Engine
{
	AnimatedModel::AnimatedModel()
	{
		//AddReference();
		type = ModelType::ANIMATED;
		animController = nullptr;
		castShadows = true;
		originalAABB.min = glm::vec3();
		originalAABB.max = glm::vec3();
		lodDistance = 10000.0f;
		rootBone = new Bone;

		for (unsigned short i = 0; i < MAX_BONES_CONNECTIONS; i++)
		{
			boneAttachments[i] = {};
			boneAttachments[i].attachedEntity = { std::numeric_limits<unsigned int>::max() };
			boneAttachments[i].boneTransformEntity = { std::numeric_limits<unsigned int>::max() };
		}
	}

	AnimatedModel::AnimatedModel(Renderer *renderer, ScriptManager &scriptManager, const std::string &path, const std::vector<std::string> &matNames)
	{
		//AddReference();
		type = ModelType::ANIMATED;
		this->path = path;
		animController = nullptr;
		castShadows = true;
		originalAABB.min = glm::vec3();
		originalAABB.max = glm::vec3();
		lodDistance = 10000.0f;
		rootBone = new Bone;

		for (unsigned short i = 0; i < MAX_BONES_CONNECTIONS; i++)
		{
			boneAttachments[i] = {};
			boneAttachments[i].attachedEntity = { std::numeric_limits<unsigned int>::max() };
			boneAttachments[i].boneTransformEntity = { std::numeric_limits<unsigned int>::max() };
		}

		LoadModel(renderer, scriptManager, matNames);
	}

	AnimatedModel::~AnimatedModel()
	{
		for (size_t i = 0; i < animations.size(); i++)
		{
			animations[i]->RemoveReference();
		}

		if (rootBone)
		{
			delete rootBone;
			rootBone = nullptr;
		}
	}

	void AnimatedModel::Dispose(Game *game)
	{
		for (unsigned short i = 0; i < curBoneAttachments; i++)
			game->GetEntityManager().Destroy(boneAttachments[i].boneTransformEntity);
	}

	void AnimatedModel::LoadModel(Renderer *renderer, ScriptManager &scriptManager, const std::vector<std::string> &matNames)
	{
		Serializer s(renderer->GetFileManager());
		s.OpenForReading(path);

		if (!s.IsOpen())
		{
			Log::Print(LogLevel::LEVEL_ERROR, "ERROR -> Failed to open model file: %s\n", path.c_str());
			return;
		}

		int magic = 0;
		s.Read(magic);

		if (magic != 314)
		{
			Log::Print(LogLevel::LEVEL_ERROR, "ERROR -> Unknown animated model file: %s\n", path.c_str());
			return;
		}

		unsigned int numMeshes = 0;
		s.Read(numMeshes);
		s.Read(globalInvTransform);

		Log::Print(LogLevel::LEVEL_INFO, "num meshes: %u\n", numMeshes);

		meshesAndMaterials.resize((size_t)numMeshes);

		for (size_t i = 0; i < meshesAndMaterials.size(); i++)
		{
			unsigned int numVertices, numIndices = 0;
			s.Read(numVertices);
			s.Read(numIndices);

			std::vector<VertexPOS3D_UV_NORMAL_BONES> vertices(numVertices);
			std::vector<unsigned short> indices(numIndices);
			s.Read(vertices.data(), (unsigned int)vertices.size() * sizeof(VertexPOS3D_UV_NORMAL_BONES));
			s.Read(indices.data(), (unsigned int)indices.size() * sizeof(unsigned short));

			VertexAttribute attribs[5] = {};
			attribs[0].count = 3;								// Position
			attribs[1].count = 2;								// UV
			attribs[2].count = 3;								// Normal
			attribs[3].count = 4;								// Bone Id's
			attribs[3].vertexAttribFormat = VertexAttributeFormat::INT;
			attribs[4].count = 4;								// Weights

			attribs[0].offset = 0;
			attribs[1].offset = 3 * sizeof(float);
			attribs[2].offset = 5 * sizeof(float);
			attribs[3].offset = 8 * sizeof(float);
			attribs[4].offset = 12 * sizeof(float);

			VertexInputDesc desc = {};
			desc.stride = sizeof(VertexPOS3D_UV_NORMAL_BONES);
			desc.attribs = { attribs[0], attribs[1], attribs[2], attribs[3], attribs[4] };

			Buffer *vb = renderer->CreateVertexBuffer(vertices.data(), vertices.size() * sizeof(VertexPOS3D_UV_NORMAL_BONES), BufferUsage::STATIC);
			Buffer *ib = renderer->CreateIndexBuffer(indices.data(), indices.size() * sizeof(unsigned short), BufferUsage::STATIC);

			Mesh m = {};
			m.vao = renderer->CreateVertexArray(desc, vb, ib);
			m.vertexOffset = 0;
			m.indexCount = indices.size();
			m.indexOffset = 0;
			m.instanceCount = 0;
			m.instanceOffset = 0;

			MeshMaterial mm = {};
			mm.mesh = m;

			std::string matName;

			if (matNames.size() == 0)
			{
				mm.mat = renderer->CreateMaterialInstance(scriptManager, "Data/Materials/modelDefaultAnimated.mat", { desc });
				matName = "modelDefault";
			}
			else
			{
				mm.mat = renderer->CreateMaterialInstance(scriptManager, matNames[i], { desc });
				matName = matNames[i].substr(matNames[i].find_last_of('/') + 1);
				// Remove the extension
				matName.pop_back();
				matName.pop_back();
				matName.pop_back();
				matName.pop_back();
			}

			strncpy(mm.mat->name, matName.c_str(), 64);

			meshesAndMaterials[i] = mm;

			Log::Print(LogLevel::LEVEL_INFO, "Mesh loaded\n");
		}

		unsigned int numBones = 0;

		s.Read(numBones);
		
		boneOffsetMatrices.resize((size_t)numBones);
		s.Read(boneOffsetMatrices.data(), boneOffsetMatrices.size() * sizeof(glm::mat4));

		boneTransforms.resize((size_t)numBones);

		std::string boneName;
		unsigned int boneIndex;
		for (unsigned int i = 0; i < numBones; i++)
		{
			s.Read(boneName);
			s.Read(boneIndex);
			boneMap[boneName] = boneIndex;
			boneTransforms[i] = glm::mat4(1.0f);
		}

		ReadBoneTree(s, rootBone);

		s.Close();
	}

	void AnimatedModel::ReadBoneTree(Serializer &s, Bone *bone)
	{
		s.Read(bone->name);
		s.Read(bone->transformation);
		unsigned int numChildren = 0;
		s.Read(numChildren);

		if (numChildren > 0)
		{
			bone->children.resize(numChildren);

			for (size_t i = 0; i < bone->children.size(); i++)
			{
				bone->children[i] = new Bone;
				bone->children[i]->parent = bone;
				ReadBoneTree(s, bone->children[i]);
			}
		}
	}

	unsigned int AnimatedModel::FindKeyFrame(float animTime, const KeyFrameList &keyFrameList)
	{
		for (unsigned int i = 0; i < keyFrameList.size() - 1; i++)
		{
			if (animTime < (float)keyFrameList[i + 1].time)
				return i;
		}

		return 0;
	}

	void AnimatedModel::Interpolate(float animTime, glm::vec3 &position, glm::quat &rot, glm::vec3 &scale, const KeyFrameList &keyFrameList)
	{
		if (keyFrameList.size() == 1)
		{
			position = keyFrameList[0].position;
			rot = keyFrameList[0].rotation;
			scale = keyFrameList[0].scale;
			return;
		}

		unsigned int index = FindKeyFrame(animTime, keyFrameList);
		unsigned int nextIndex = index + 1;

		curKeyFrame = static_cast<unsigned short>(index);

		float deltaTime = (float)(keyFrameList[nextIndex].time - keyFrameList[index].time);
		float factor = (animTime - (float)keyFrameList[index].time) / deltaTime;

		// Position
		const glm::vec3 &startPos = keyFrameList[index].position;
		const glm::vec3 &endPos = keyFrameList[nextIndex].position;

		glm::vec3 deltaPos = endPos - startPos;
		position = startPos + factor * deltaPos;

		// Rotation
		const glm::quat &startRot = keyFrameList[index].rotation;
		const glm::quat &endRot = keyFrameList[nextIndex].rotation;
		rot = glm::slerp(startRot, endRot, factor);
		rot = glm::normalize(rot);

		// Scale
		const glm::vec3 &startScale = keyFrameList[index].scale;
		const glm::vec3 &endScale = keyFrameList[nextIndex].scale;

		glm::vec3 deltaScale = endScale - startScale;
		scale = startScale + factor * deltaScale;
	}

	void AnimatedModel::UpdateBoneTree(float curAnimTime, float nextAnimTime, const Bone *bone, const glm::mat4 &parentTransform)
	{
		glm::mat4 nodeTransformation = bone->transformation;

		glm::vec3 curAnimPos = glm::vec3(0.0f);
		glm::vec3 curAnimScale = glm::vec3(1.0f);
		glm::quat curAnimRot = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);

		glm::vec3 nextAnimPos = glm::vec3(0.0f);
		glm::vec3 nextAnimScale = glm::vec3(1.0f);
		glm::quat nextAnimRot = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);

		if (shouldTransition || shouldRevert)
		{
			bool can = false;
			bool can2 = false;
			const KeyFrameList &curAnimKeyframes = animations[currentAnimation]->bonesKeyframesList[bone->name];
			const KeyFrameList &nextAnimKeyframes = animations[nextAnim]->bonesKeyframesList[bone->name];

			if (curAnimKeyframes.size() >= 1)
			{
				Interpolate(curAnimTime, curAnimPos, curAnimRot, curAnimScale, curAnimKeyframes);
				can = true;
			}
			if (nextAnimKeyframes.size() >= 1)
			{
				Interpolate(nextAnimTime, nextAnimPos, nextAnimRot, nextAnimScale, nextAnimKeyframes);
				can2 = true;
			}

			if (can && can2)
			{
				glm::vec3 interpPos = glm::mix(curAnimPos, nextAnimPos, blendFactor);
				glm::quat intertRot = glm::slerp(curAnimRot, nextAnimRot, blendFactor);
				intertRot = glm::normalize(intertRot);
				//glm::vec3 interpScale = glm::mix(curAnimScale, nextAnimScale, blendFactor);

				glm::mat4 transM = glm::translate(glm::mat4(1.0f), interpPos);
				glm::mat4 rotM = glm::mat4_cast(intertRot);
				//glm::mat4 scaleM = glm::scale(glm::mat4(1.0f), interpScale);

				nodeTransformation = transM * rotM;
			}
		}
		else
		{
			const KeyFrameList &curAnimKeyframes = animations[currentAnimation]->bonesKeyframesList[bone->name];

			if (curAnimKeyframes.size() >= 1)
			{
				Interpolate(curAnimTime, curAnimPos, curAnimRot, curAnimScale, curAnimKeyframes);

				glm::mat4 transM = glm::translate(glm::mat4(1.0f), curAnimPos);
				glm::mat4 rotM = glm::mat4_cast(curAnimRot);
				//glm::mat4 scaleM = glm::scale(glm::mat4(1.0f), curAnimScale);

				nodeTransformation = transM * rotM;
			}
		}

		glm::mat4 globalTransformation = parentTransform * nodeTransformation;

		if (boneMap.find(bone->name) != boneMap.end())
		{
			unsigned int boneIndex = boneMap[bone->name];
			// Multiply with the offset matrix to bring the bone from local space to bone space
			// Then multiply this bone's transformation and all it's parents
			boneTransforms[boneIndex] = globalInvTransform * globalTransformation * boneOffsetMatrices[boneIndex];
			//boneTransforms[boneIndex] = glm::mat4(1.0f);
		}

		for (unsigned int i = 0; i < bone->children.size(); i++)
		{
			UpdateBoneTree(curAnimTime, nextAnimTime, bone->children[i], globalTransformation);
		}
	}

	void AnimatedModel::GetBoneTransform(const Bone *bone, glm::mat4 &transform)
	{
		if (boneMap.find(bone->name) != boneMap.end())
		{
			glm::mat4 m = bone->transformation;				// The bone's x position is flipped. Handedness different?
			m[3].x = -m[3].x;
			transform = globalInvTransform * m * transform;
		}

		if (bone->parent)
			GetBoneTransform(bone->parent, transform);
	}

	void AnimatedModel::AddAnimation(Animation *anim)
	{
		if (!anim)
			return;

		animations.push_back(anim);
		anim->AddReference();
	}

	void AnimatedModel::SetAnimationController(FileManager *fileManager, const std::string &controllerPath, ModelManager *modelManager)
	{
		if (animController)
		{
			delete animController;
			animController = nullptr;
		}

		animController = new AnimationController(controllerPath);

		Serializer s(fileManager);
		s.OpenForReading(controllerPath);
		animController->Deserialize(s, modelManager);
		s.Close();
	}

	unsigned short AnimatedModel::AddBoneAttachment(Game *game, Bone *bone, Entity attachedEntity)
	{
		if (curBoneAttachments >= MAX_BONES_CONNECTIONS || !bone)
			return -1;			// Return -1 as the index (which will go the max ushort value)

		for (unsigned short i = 0; i < curBoneAttachments; i++)
		{
			if (boneAttachments[i].bone == bone)			// Only 1 attachment per bone
				return -1;
		}

		BoneAttachment bc = {};
		bc.attachedEntity = attachedEntity;
		bc.bone = bone;
		bc.boneTransformEntity = game->AddEntity();
		//obj->SetParent(bc.boneTransform);
		//attachedEntity setparent(bonetransformentity)

		boneAttachments[curBoneAttachments] = bc;

		unsigned short boneAttachID = curBoneAttachments;
		curBoneAttachments++;

		return boneAttachID;
	}

	unsigned short AnimatedModel::AddBoneAttachment(Game *game, const std::string &boneName, Entity attachedEntity)
	{
		Bone *bone = nullptr;
		FindBoneByName(rootBone, boneName, &bone);

		if (curBoneAttachments >= MAX_BONES_CONNECTIONS || !bone)
			return -1;			// Return -1 as the index (which will go the max ushort value)

		for (unsigned short i = 0; i < curBoneAttachments; i++)
		{
			if (boneAttachments[i].bone == bone)			// Only 1 attachment per bone
				return -1;
		}

		BoneAttachment bc = {};
		bc.attachedEntity = attachedEntity;
		bc.bone = bone;
		bc.boneTransformEntity = game->AddEntity();
		//obj->SetParent(bc.boneTransform);
		//attachedEntity setparent(bonetransformentity)

		boneAttachments[curBoneAttachments] = bc;

		unsigned short boneAttachID = curBoneAttachments;
		curBoneAttachments++;

		return boneAttachID;
	}

	void AnimatedModel::SwitchBoneAttachmentEntity(TransformManager &transformManager, unsigned short boneAttachID, Entity newAttachedEntity)
	{
		if (boneAttachID >= curBoneAttachments)
			return;

		//boneAttachments[boneAttachID].boneTransform->RemoveChild(boneAttachments[boneAttachID].obj);
		transformManager.RemoveParent(newAttachedEntity);

		boneAttachments[boneAttachID].attachedEntity = newAttachedEntity;
		transformManager.SetParent(newAttachedEntity, boneAttachments[boneAttachID].boneTransformEntity);
		//obj->SetParent(boneAttachments[boneAttachID].boneTransform);
	}

	void AnimatedModel::RemoveBoneAttachment(EntityManager &entityManager, const BoneAttachment *boneAttach)
	{
		if (!boneAttach)
			return;

		BoneAttachment *curAttach = boneAttachments;
		for (size_t i = 0; i < curBoneAttachments; i++)
		{
			if (curAttach == boneAttach)
			{
				BoneAttachment &ba = boneAttachments[i];

				entityManager.Destroy(boneAttachments[i].boneTransformEntity);

				ba.bone = nullptr;
				ba.attachedEntity = { std::numeric_limits<unsigned int>::max() };
				ba.boneTransformEntity = { std::numeric_limits<unsigned int>::max() };

				curBoneAttachments--;
			}
			curAttach++;
		}
	}

	void AnimatedModel::RemoveBoneAttachments(TransformManager &transformManager)
	{
		for (size_t i = 0; i < curBoneAttachments; i++)
			transformManager.RemoveParent(boneAttachments[i].attachedEntity);
	}

	void AnimatedModel::FillBoneAttachments(Game *game)
	{
		// Can now be done in deserialize
		/*for (unsigned short i = 0; i < curBoneAttachments; i++)
		{
			//boneAttachments[i].obj = game->GetSceneManager()->GetObjectById(boneAttachments[i].objID);		// replace with entity
			boneAttachments[i].attachedEntity = 
			boneAttachments[i].boneTransformEntity = game->AddEntity();
		}*/
	}

	void AnimatedModel::ReplaceBoneAttachmentEntity(unsigned int boneAttachID, Entity newAttachedEntity)
	{
		if (boneAttachID < curBoneAttachments)
			boneAttachments[boneAttachID].attachedEntity = newAttachedEntity;
	}

	void AnimatedModel::UpdateController()
	{
		if (animController)
			animController->Update(this);
	}

	void AnimatedModel::UpdateBones(TransformManager &transformManager, Entity self, float deltaTime)
	{
		isDirty = false;

		if (animations.size() > 0 && !isPaused)
		{
			elapsedTime += deltaTime;

			// If we replaced the cur anim after the anim times calculation below, the times would be wrong because they would have used the previous animation as the current
			if (blendFactor >= 1.0f)
			{
				currentAnimation = nextAnim;
			}

			float curTimeInTicks = elapsedTime * animations[currentAnimation]->ticksPerSecond;
			float curAnimTime = fmodf(curTimeInTicks, animations[currentAnimation]->duration);

			float nextTimeInTicks = elapsedTime * animations[nextAnim]->ticksPerSecond;
			float nextAnimTime = fmodf(nextTimeInTicks, animations[nextAnim]->duration);

			// Moved the if up here instead of right after when we set isAnimationFinished to true so we can pause an animation that is not looping at the end otherwise it would go to the initial pose
			// We could force curAnimTime to be equal to the anim duration but there would be artifacts
			if (isAnimationFinished)
			{
				if (!loopCurrent)		// If this anim is not looping and it has finished then don't let it continue by setting curAnimTime to 0
				{
					curAnimTime = 0.0f;
				}
			}

			if (shouldTransition)
			{
				if (blendFactor >= 1.0f)
				{
					shouldTransition = false;
					isTransitionFinished = true;
					//currentAnimation = nextAnim;

					if (!loopNext)		// If we just finished the transition to the next anim and that anim is not looping then reset the time and curAnimTime so we start at the first frame
					{
						elapsedTime = 0.0f;
						curAnimTime = 0.0f;
					}

					animTime = 0.0f;
					loopCurrent = loopNext;
					isAnimationFinished = false;
				}

				blendFactor = currentTime / transitionTime;
				currentTime += deltaTime;

				if (!loopNext)		// If we're transitioning and the next animation is not going to loop then transition to the first frame of it
				{
					nextAnimTime = 0.0f;
				}
			}
			else if (shouldRevert)
			{
				if (blendFactor <= 0.0f)
				{
					shouldRevert = false;
					animTime = 0.0f;
					isAnimationFinished = false;
				}

				blendFactor = currentTime / transitionTime;
				currentTime -= deltaTime;
			}

			if (blendFactor >= 1.0f)
				animTime += deltaTime;

			if (animTime >= animations[currentAnimation]->duration / animations[currentAnimation]->ticksPerSecond)
			{
				isAnimationFinished = true;				
			}

			glm::mat4 identity = glm::mat4(1.0f);
			UpdateBoneTree(curAnimTime, nextAnimTime, rootBone, identity);
		}

		// Update bone attachments
		for (unsigned short i = 0; i < curBoneAttachments; i++)
		{
			unsigned int boneIndex = boneMap[boneAttachments[i].bone->name];
			glm::mat4 curBoneTransform = glm::mat4(1.0f);

			GetBoneTransform(boneAttachments[i].bone, curBoneTransform);
			curBoneTransform = /*self->GetLocalToWorldTransform() **/ boneTransforms[boneIndex] * curBoneTransform;

			if (transformManager.HasParent(boneAttachments[i].boneTransformEntity) == false)
				transformManager.SetParent(boneAttachments[i].boneTransformEntity, self);

			transformManager.SetLocalToParent(boneAttachments[i].boneTransformEntity, curBoneTransform);

			if (transformManager.HasParent(boneAttachments[i].attachedEntity) == false)
				transformManager.SetParent(boneAttachments[i].attachedEntity, boneAttachments[i].boneTransformEntity);
		}
	}

	void AnimatedModel::PlayAnimationStr(const std::string &name)
	{
		currentAnimation = 0;
	}

	void AnimatedModel::PlayAnimation(unsigned int index)
	{
		if (index >= animations.size())
			return;

		currentAnimation = index;
		blendFactor = 1.0f;
		nextAnim = currentAnimation;
	}

	void AnimatedModel::PauseAnimation(bool pause)
	{
		isPaused = pause;
	}

	void AnimatedModel::StopAnimation(unsigned int index)
	{
	}

	void AnimatedModel::TransitionTo(unsigned int index, float time, bool loop)
	{
		if (index >= animations.size())
			return;

		nextAnim = index;
		transitionTime = time;
		shouldTransition = true;
		currentTime = 0.0f;
		blendFactor = 0.0f;
		animTime = 0.0f;
		loopNext = loop;
		isTransitionFinished = false;
		//isAnimationFinished = false;
	}

	void AnimatedModel::TransitionTo(Animation *anim, float time, bool loop)
	{
		for (size_t i = 0; i < animations.size(); i++)
		{
			if (animations[i] == anim)
			{
				nextAnim = i;
				transitionTime = time;
				shouldTransition = true;
				currentTime = 0.0f;
				blendFactor = 0.0f;
				animTime = 0.0f;
				loopNext = loop;
				isTransitionFinished = false;
				//isAnimationFinished = false;
				break;
			}
		}
	}

	void AnimatedModel::RevertTransition()
	{
		shouldRevert = true;
		shouldTransition = false;
		isAnimationFinished = false;
	}

	void AnimatedModel::FindBoneByName(Bone *startingBone, const std::string &boneName, Bone **outBone)
	{
		if (startingBone->name == boneName)
		{
			*outBone = startingBone;
			return;
		}

		for (size_t i = 0; i < startingBone->children.size(); i++)
		{
			FindBoneByName(startingBone->children[i], boneName, outBone);
		}
	}

	void AnimatedModel::Serialize(Serializer &s)
	{
		s.Write(path);
		s.Write(castShadows);
		s.Write(lodDistance);
		s.Write(originalAABB.min);
		s.Write(originalAABB.max);

		s.Write((unsigned int)meshesAndMaterials.size());

		for (size_t i = 0; i < meshesAndMaterials.size(); i++)
			s.Write(meshesAndMaterials[i].mat->path);

		s.Write(animations.size());

		for (size_t i = 0; i < animations.size(); i++)
		{
			s.Write(animations[i]->path);
		}

		if (animController)
		{
			s.Write(true);
			s.Write(animController->GetPath());
		}
		else
			s.Write(false);

		s.Write(curBoneAttachments);
		for (unsigned short i = 0; i < curBoneAttachments; i++)
		{
			s.Write(boneAttachments[i].bone->name);
			s.Write(boneAttachments[i].attachedEntity.id);
		}
	}

	void AnimatedModel::Deserialize(Serializer &s, Game *game, bool reload)
	{
		s.Read(path);
		s.Read(castShadows);
		s.Read(lodDistance);
		s.Read(originalAABB.min);
		s.Read(originalAABB.max);

		unsigned int matCount;
		s.Read(matCount);

		std::vector<std::string> matNames(matCount);
		for (unsigned int i = 0; i < matCount; i++)
			s.Read(matNames[i]);

		if (!reload)
			LoadModel(game->GetRenderer(), game->GetScriptManager(), matNames);

		unsigned int numAnimations = 0;
		s.Read(numAnimations);

		for (unsigned int i = 0; i < numAnimations; i++)
		{
			std::string path;
			s.Read(path);

			bool alreadyLoaded = false;

			// Check if we already have the animation stored (needed for when the game is stopped in the editor so we don't keep (re)loading the same animations over and over
			for (size_t j = 0; j < animations.size(); j++)
			{
				if (animations[j]->path == path)
				{
					alreadyLoaded = true;
					break;
				}
			}
			if (alreadyLoaded)
				continue;

			if (!reload)
			{
				Animation *a = game->GetModelManager().LoadAnimation(path);
				a->AddReference();
				animations.push_back(a);
			}
		}

		bool hasAnimController = false;

		s.Read(hasAnimController);

		if (hasAnimController)
		{
			std::string path;
			s.Read(path);
			if (!reload)
				SetAnimationController(game->GetFileManager(), path, &game->GetModelManager());
		}

		s.Read(curBoneAttachments);
		std::string boneName;

		// curBoneAttachments might be 0 after deserializing but we could have added bone attachments during play mode, so we need to reset all the bone attachments
		if (curBoneAttachments == 0)
		{
			for (unsigned short i = 0; i < MAX_BONES_CONNECTIONS; i++)
			{
				game->GetEntityManager().Destroy(boneAttachments[i].boneTransformEntity);

				boneAttachments[i] = {};
				boneAttachments[i].attachedEntity = { std::numeric_limits<unsigned int>::max() };
				boneAttachments[i].boneTransformEntity = { std::numeric_limits<unsigned int>::max() };
			}
		}
		else
		{
			for (unsigned short i = 0; i < curBoneAttachments; i++)
			{
				s.Read(boneName);
				FindBoneByName(rootBone, boneName, &boneAttachments[i].bone);

				s.Read(boneAttachments[i].attachedEntity.id);
			}
		}
	}
}
