#include "AnimatedModel.h"

#include "Graphics\ResourcesLoader.h"
#include "Graphics\VertexArray.h"
#include "Graphics\Buffers.h"
#include "Graphics\Renderer.h"
#include "Program\Log.h"
#include "Game\Game.h"
#include "Program\Input.h"
#include "Graphics\Material.h"

#include "include\glm\gtc\matrix_transform.hpp"
#include "include\glm\glm.hpp"
#include "include\glm\gtc\type_ptr.hpp"

#include "include\assimp\postprocess.h"

#include <iostream>

namespace Engine
{
	AnimatedModel::AnimatedModel()
	{
		type = ModelType::ANIMATED;
		animController = nullptr;
		isInstanced = false;

		for (unsigned short i = 0; i < MAX_BONES_CONNECTIONS; i++)
		{
			boneAttachments[i] = {};
			boneAttachments[i].attachedEntity = { std::numeric_limits<unsigned int>::max() };
			boneAttachments[i].boneTransformEntity = { std::numeric_limits<unsigned int>::max() };
		}
	}

	AnimatedModel::AnimatedModel(Game *game, const std::string &path)
	{
		type = ModelType::ANIMATED;
		animController = nullptr;
		isInstanced = false;
		this->path = path;

		for (unsigned short i = 0; i < MAX_BONES_CONNECTIONS; i++)
		{
			boneAttachments[i] = {};
			boneAttachments[i].attachedEntity = { std::numeric_limits<unsigned int>::max() };
			boneAttachments[i].boneTransformEntity = { std::numeric_limits<unsigned int>::max() };
		}

		const std::vector<std::string> matNames = {};
		LoadModelFile(game, game->GetRenderer(), matNames, game->GetScriptManager(), false);
	}

	AnimatedModel::AnimatedModel(Game *game, const std::string &path, const std::vector<std::string> &matNames)
	{
		type = ModelType::ANIMATED;
		animController = nullptr;
		isInstanced = false;
		this->path = path;

		for (unsigned short i = 0; i < MAX_BONES_CONNECTIONS; i++)
		{
			boneAttachments[i] = {};
			boneAttachments[i].attachedEntity = { std::numeric_limits<unsigned int>::max() };
			boneAttachments[i].boneTransformEntity = { std::numeric_limits<unsigned int>::max() };
		}

		LoadModelFile(game, game->GetRenderer(), matNames, game->GetScriptManager(), false);
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

	void AnimatedModel::LoadModelFile(Game *game, Renderer *renderer, const std::vector<std::string>& matNames, ScriptManager &scriptManager, bool loadVertexColors)
	{
		type = ModelType::ANIMATED;

		originalAABB.min = glm::vec3(100000.0f);
		originalAABB.max = glm::vec3(-100000.0f);

		//const aiScene* scene = importer.ReadFile(path, aiProcess_JoinIdenticalVertices | aiProcess_FlipUVs | aiProcess_GenSmoothNormals); //| aiProcess_CalcTangentSpace);
		const aiScene *scene = importer.ReadFile(path, aiProcess_JoinIdenticalVertices | aiProcess_FlipUVs | aiProcess_GenSmoothNormals | aiProcess_LimitBoneWeights); //| aiProcess_CalcTangentSpace);

		if (!scene || scene->mFlags == AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
		{
			std::cout << "Assimp Error: " << importer.GetErrorString() << "\n";
		}

		const aiMatrix4x4 &aim = scene->mRootNode->mTransformation;
		globalInvTransform[0] = glm::vec4(aim.a1, aim.b1, aim.c1, aim.d1);
		globalInvTransform[1] = glm::vec4(aim.a2, aim.b2, aim.c2, aim.d2);
		globalInvTransform[2] = glm::vec4(aim.a3, aim.b3, aim.c3, aim.d3);
		globalInvTransform[3] = glm::vec4(aim.a4, aim.b4, aim.c4, aim.d4);
		globalInvTransform = glm::inverse(globalInvTransform);

		// Load all the model meshes
		for (unsigned int i = 0; i < scene->mNumMeshes; i++)
		{
			const aiMesh* aiMesh = scene->mMeshes[i];

			if (matNames.size() > 0)
			{
				MeshMaterial mm;
				mm.mesh = ProcessMesh(renderer, aiMesh, scene, loadVertexColors);
				mm.mat = renderer->CreateMaterialInstance(scriptManager, matNames[meshesAndMaterials.size()], mm.mesh.vao->GetVertexInputDescs());
				meshesAndMaterials.push_back(mm);
			}
			else
			{
				MeshMaterial mm;
				mm.mesh = ProcessMesh(renderer, aiMesh, scene, loadVertexColors);
				mm.mat = renderer->CreateMaterialInstance(scriptManager, "Data/Resources/Materials/modelDefaultAnimated.mat", mm.mesh.vao->GetVertexInputDescs());
				meshesAndMaterials.push_back(mm);
			}
		}

		for (size_t i = 0; i < scene->mNumAnimations; i++)
		{
			Animation *a = new Animation();
			a->AddReference();
			a->Store(scene->mAnimations[i]);
			a->loadedSeparately = false;

			animations.push_back(a);
			
			game->GetModelManager().AddAnimation(a, a->path + std::to_string(i));		// Add a number to the path so the id is unique otherwise the animations would be replaced	
		}

		rootBone = new Bone;
		rootBone->parent = nullptr;
		rootBone->name = std::string(scene->mRootNode->mName.data);

		glm::mat4 nodeTransformation;
		nodeTransformation[0] = glm::vec4(aim.a1, aim.b1, aim.c1, aim.d1);
		nodeTransformation[1] = glm::vec4(aim.a2, aim.b2, aim.c2, aim.d2);
		nodeTransformation[2] = glm::vec4(aim.a3, aim.b3, aim.c3, aim.d3);
		nodeTransformation[3] = glm::vec4(aim.a3, aim.b4, aim.c4, aim.d4);

		rootBone->transformation = nodeTransformation;

		for (size_t i = 0; i < scene->mRootNode->mNumChildren; i++)
		{
			BuildBoneTree(scene->mRootNode->mChildren[i], rootBone);
		}
	}

	Mesh AnimatedModel::ProcessMesh(Renderer *renderer, const aiMesh *aimesh, const aiScene *aiscene, bool loadVertexColors)
	{
		std::vector<VertexPOS3D_UV_NORMAL_BONES> vertices(aimesh->mNumVertices);
		std::vector<unsigned short> indices;

		for (unsigned int i = 0; i < aimesh->mNumVertices; i++)
		{
			VertexPOS3D_UV_NORMAL_BONES vertex = {};

			vertex.pos = glm::vec3(aimesh->mVertices[i].x, aimesh->mVertices[i].y, aimesh->mVertices[i].z);
			vertex.normal = glm::vec3(aimesh->mNormals[i].x, aimesh->mNormals[i].y, aimesh->mNormals[i].z);

			originalAABB.min = glm::min(originalAABB.min, vertex.pos);
			originalAABB.max = glm::max(originalAABB.max, vertex.pos);

			if (aimesh->mTextureCoords[0])
			{
				// A vertex can contain up to 8 different texture coordinates. We thus make the assumption that we won't
				// use models where a vertex can have multiple texture coordinates so we always take the first set (0).
				vertex.uv = glm::vec2(aimesh->mTextureCoords[0][i].x, aimesh->mTextureCoords[0][i].y);
			}
			else
			{
				vertex.uv = glm::vec2(0.0f, 0.0f);
			}

			vertex.weights = glm::vec4(0.0f);

			vertices[i] = vertex;
		}

		for (unsigned int i = 0; i < aimesh->mNumFaces; i++)
		{
			aiFace face = aimesh->mFaces[i];

			for (unsigned int j = 0; j < face.mNumIndices; ++j)
			{
				indices.push_back(face.mIndices[j]);
			}
		}

		for (unsigned int i = 0; i < aimesh->mNumBones; i++)
		{
			unsigned int boneIndex = 0;

			std::string boneName(aimesh->mBones[i]->mName.data);

			// Check if we've already loaded this bone's info, if not, then load it
			if (boneMap.find(boneName) == boneMap.end())
			{
				boneIndex = numBones;
				numBones++;

				aiMatrix4x4 &aim = aimesh->mBones[i]->mOffsetMatrix;
				glm::mat4 boneOffset = glm::mat4(1.0f);
				boneOffset[0] = glm::vec4(aim.a1, aim.b1, aim.c1, aim.d1);
				boneOffset[1] = glm::vec4(aim.a2, aim.b2, aim.c2, aim.d2);
				boneOffset[2] = glm::vec4(aim.a3, aim.b3, aim.c3, aim.d3);
				boneOffset[3] = glm::vec4(aim.a4, aim.b4, aim.c4, aim.d4);

				boneOffsetMatrices.push_back(boneOffset);
				boneTransforms.push_back(glm::mat4(1.0f));
				boneMap[boneName] = boneIndex;
			}
			else
			{
				boneIndex = boneMap[boneName];
			}

			for (unsigned int j = 0; j < aimesh->mBones[i]->mNumWeights; j++)
			{
				unsigned int vertexID = aimesh->mBones[i]->mWeights[j].mVertexId;

				VertexPOS3D_UV_NORMAL_BONES &v = vertices[vertexID];

				// Find a free slot and store the bone ID and weight
				for (unsigned int k = 0; k < MAX_BONES_PER_VERTEX; k++)
				{
					if (v.weights[k] == 0.0f)
					{
						v.boneIDs[k] = boneIndex;
						v.weights[k] = aimesh->mBones[i]->mWeights[j].mWeight;
						break;
					}
				}
			}
		}

		// Prevent the min and max from being both 0 (when loading a plane for example)
		if (originalAABB.min.y < 0.001f && originalAABB.min.y > -0.001f)
			originalAABB.min.y = -0.01f;
		if (originalAABB.max.y < 0.001f && originalAABB.max.y > -0.001f)
			originalAABB.max.y = 0.01f;

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

		return m;
	}

	bool AnimatedModel::FindBone(aiAnimation *anim, const std::string &nodeName)
	{
		for (unsigned int i = 0; i < anim->mNumChannels; i++)
		{
			const aiNodeAnim* nodeAnim = anim->mChannels[i];

			if (std::string(nodeAnim->mNodeName.data) == nodeName)
			{
				return true;
			}
		}
		return false;
	}

	void AnimatedModel::BuildBoneTree(aiNode *node, Bone *parent)
	{
		Bone *bone = new Bone;
		bone->parent = parent;
		bone->name = std::string(node->mName.data);

		const aiMatrix4x4 &aim = node->mTransformation;

		glm::mat4 nodeTransformation;
		nodeTransformation[0] = glm::vec4(aim.a1, aim.b1, aim.c1, aim.d1);
		nodeTransformation[1] = glm::vec4(aim.a2, aim.b2, aim.c2, aim.d2);
		nodeTransformation[2] = glm::vec4(aim.a3, aim.b3, aim.c3, aim.d3);
		nodeTransformation[3] = glm::vec4(aim.a3, aim.b4, aim.c4, aim.d4);

		bone->transformation = nodeTransformation;

		parent->children.push_back(bone);

		for (unsigned int i = 0; i < node->mNumChildren; i++)
		{
			BuildBoneTree(node->mChildren[i], bone);
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

	void AnimatedModel::SetAnimationController(const std::string &controllerPath, ModelManager *modelManager)
	{
		if (animController)
		{
			delete animController;
			animController = nullptr;
		}

		animController = new AnimationController(controllerPath);

		Serializer s;
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
			float curAnimTime = std::fmodf(curTimeInTicks, animations[currentAnimation]->duration);

			float nextTimeInTicks = elapsedTime * animations[nextAnim]->ticksPerSecond;
			float nextAnimTime = std::fmodf(nextTimeInTicks, animations[nextAnim]->duration);

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
		s.Write(isInstanced);
		s.Write(castShadows);
		s.Write(lodDistance);

		s.Write((unsigned int)meshesAndMaterials.size());
		for (size_t i = 0; i < meshesAndMaterials.size(); i++)
			s.Write(meshesAndMaterials[i].mat->path);

		s.Write(animations.size());

		for (size_t i = 0; i < animations.size(); i++)
		{
			s.Write(animations[i]->loadedSeparately);
			if (animations[i]->loadedSeparately)
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
		s.Read(isInstanced);
		s.Read(castShadows);
		s.Read(lodDistance);

		unsigned int matCount;
		s.Read(matCount);

		std::vector<std::string> matNames(matCount);
		for (unsigned int i = 0; i < matCount; i++)
			s.Read(matNames[i]);

		if (!reload)
			LoadModelFile(game, game->GetRenderer(), matNames, game->GetScriptManager(), false);

		unsigned int size = 0;
		s.Read(size);

		for (unsigned int i = 0; i < size; i++)
		{
			bool loadedSeparately = false;
			s.Read(loadedSeparately);

			if (loadedSeparately)
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
		}

		bool hasAnimController = false;

		s.Read(hasAnimController);

		if (hasAnimController)
		{
			std::string path;
			s.Read(path);
			if (!reload)
				SetAnimationController(path, &game->GetModelManager());
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
