#include "AssimpLoader.h"

#include "Program/Log.h"
#include "Program/FileManager.h"

#include "Game/Game.h"

#include "Graphics/Model.h"
#include "Graphics/Material.h"
#include "Graphics/Renderer.h"
#include "Graphics/VertexArray.h"
#include "Graphics/Animation/AnimatedModel.h"

#include "include/assimp/scene.h"
#include "include/assimp/Importer.hpp"
#include "include/assimp/postprocess.h"

#include "include/glm/gtc/matrix_transform.hpp"

namespace Engine
{
	void AssimpLoader::LoadModel(Game *game, const std::string &path, const std::vector<std::string> &matPaths, bool willUseInstancing, bool isAnimated, bool loadVertexColors)
	{
		Renderer *renderer = game->GetRenderer();
		ScriptManager &scriptManager = game->GetScriptManager();

		Model *model = new Model();
		model->SetOriginalAABB({ glm::vec3(100000.0f), glm::vec3(-100000.0f) });

		Assimp::Importer importer;
		const aiScene* aiscene = importer.ReadFile(path, aiProcess_JoinIdenticalVertices | aiProcess_FlipUVs); //| aiProcess_GenSmoothNormals); //| aiProcess_CalcTangentSpace);

		if (!aiscene || aiscene->mFlags == AI_SCENE_FLAGS_INCOMPLETE || !aiscene->mRootNode)
		{
			Log::Print(LogLevel::LEVEL_ERROR, "Assimp Error: %s\n", importer.GetErrorString());
			return;
		}

		Log::Print(LogLevel::LEVEL_INFO, "Model imported\n");
		Log::Print(LogLevel::LEVEL_INFO, "Num Meshes: %d\n", aiscene->mNumMeshes);

		Serializer s(game->GetFileManager());
		s.OpenForWriting();

		s.Write(313);
		s.Write(aiscene->mNumMeshes);
		s.Write(2);						// Vertex type

		// Load all the model meshes
		for (unsigned int i = 0; i < aiscene->mNumMeshes; i++)
		{
			const aiMesh* aimesh = aiscene->mMeshes[i];

			aiMaterial *aiMat = nullptr;

			if (aimesh->mMaterialIndex >= 0)
				aiMat = aiscene->mMaterials[aimesh->mMaterialIndex];

			if (matPaths.size() > 0)
			{
				MeshMaterial mm;
				mm.mesh = ProcessMesh(renderer, s, model, aiscene, aimesh, willUseInstancing, loadVertexColors);

				const std::string &matPath = matPaths[model->GetMeshesAndMaterials().size()];

				if (matPath.size() > 0)
					mm.mat = renderer->CreateMaterialInstance(scriptManager, matPath, mm.mesh.vao->GetVertexInputDescs());
				else
					mm.mat = LoadMaterialFromAssimpMat(renderer, &scriptManager, path, mm.mesh, aiMat, isAnimated);

				if (aiMat)
				{
					aiString name;
					aiMat->Get(AI_MATKEY_NAME, name);
					strncpy(mm.mat->name, name.C_Str(), 64);
				}

				model->AddMeshMaterial(mm);
			}
			else
			{
				// Workaround for terrain vegetation to work when adding through the editor with no materials
				if (willUseInstancing == false)
				{
					MeshMaterial mm;
					mm.mesh = ProcessMesh(renderer, s, model, aiscene, aimesh, willUseInstancing, loadVertexColors);
					mm.mat = LoadMaterialFromAssimpMat(renderer, &scriptManager, path, mm.mesh, aiMat, isAnimated);

					if (aiMat)
					{
						aiString name;
						aiMat->Get(AI_MATKEY_NAME, name);
						strncpy(mm.mat->name, name.C_Str(), 64);
					}

					model->AddMeshMaterial(mm);
				}
				else
				{
					MeshMaterial mm;
					mm.mesh = ProcessMesh(renderer, s, model, aiscene, aimesh, willUseInstancing, loadVertexColors);
					mm.mat = renderer->CreateMaterialInstance(scriptManager, "Data/Materials/modelDefaultInstanced.mat", mm.mesh.vao->GetVertexInputDescs());

					if (aiMat)
					{
						aiString name;
						aiMat->Get(AI_MATKEY_NAME, name);
						strncpy(mm.mat->name, name.C_Str(), 64);
					}

					model->AddMeshMaterial(mm);
				}
			}
		}

		// Remove the model file extension and add our own so when we load the project again, the model will be loaded from our own custom format
		std::string p = path;
		size_t dotIndex = p.find('.', 0) + 1;		// +1 to exclude the dot
		size_t extensionSize = p.length() - dotIndex;
		for (size_t i = 0; i < extensionSize; i++)
		{
			p.pop_back();
		}

		p += "model";

		model->SetPath(p);

		s.Save(p);
		s.Close();

		delete model;
	}

	void AssimpLoader::LoadAnimatedModel(Game *game, const std::string &path, const std::vector<std::string> &matPaths)
	{
		Renderer *renderer = game->GetRenderer();
		ScriptManager &scriptManager = game->GetScriptManager();

		AnimatedModel *am = new AnimatedModel();
		am->SetOriginalAABB({ glm::vec3(100000.0f), glm::vec3(-100000.0f) });

		Assimp::Importer importer;

		const aiScene *aiscene = importer.ReadFile(path, aiProcess_JoinIdenticalVertices | aiProcess_FlipUVs | aiProcess_GenSmoothNormals | aiProcess_LimitBoneWeights); //| aiProcess_CalcTangentSpace);

		if (!aiscene || aiscene->mFlags == AI_SCENE_FLAGS_INCOMPLETE || !aiscene->mRootNode)
		{
			Log::Print(LogLevel::LEVEL_ERROR, "Assimp Error: %s\n", importer.GetErrorString());
			return;
		}

		// Remove the model file extension and add our own so when we load the project again, the model will be loaded from our own custom format
		std::string newModelPath = path;
		size_t dotIndex = newModelPath.find('.', 0) + 1;		// +1 to exclude the dot
		size_t extensionSize = newModelPath.length() - dotIndex;
		for (size_t i = 0; i < extensionSize; i++)
		{
			newModelPath.pop_back();
		}

		std::string animPath = newModelPath;
		size_t slashIndex = animPath.find_last_of('/') + 1;
		size_t modelFileNameSize = animPath.length() - slashIndex;
		for (size_t i = 0; i < modelFileNameSize; i++)
		{
			animPath.pop_back();
		}

		for (size_t i = 0; i < aiscene->mNumAnimations; i++)
		{
			Animation *a = new Animation;
			a->refCount = 0;

			StoreAnimation(a, aiscene->mAnimations[i]);

			a->path = animPath + a->name + ".anim";

			am->AddAnimation(a);

			//game->GetModelManager().AddAnimation(a, a->path);

			SerializeAnimation(game->GetFileManager(), a);
		}

		Serializer modelSerializer(game->GetFileManager());
		modelSerializer.OpenForWriting();

		modelSerializer.Write(314);
		modelSerializer.Write(aiscene->mNumMeshes);

		const aiMatrix4x4 &aim = aiscene->mRootNode->mTransformation;
		glm::mat4 globalInvTransform = glm::mat4(1.0f);
		globalInvTransform[0] = glm::vec4(aim.a1, aim.b1, aim.c1, aim.d1);
		globalInvTransform[1] = glm::vec4(aim.a2, aim.b2, aim.c2, aim.d2);
		globalInvTransform[2] = glm::vec4(aim.a3, aim.b3, aim.c3, aim.d3);
		globalInvTransform[3] = glm::vec4(aim.a4, aim.b4, aim.c4, aim.d4);
		globalInvTransform = glm::inverse(globalInvTransform);

		am->SetGlobalInvTransform(globalInvTransform);

		modelSerializer.Write(globalInvTransform);

		// Load all the model meshes
		for (unsigned int i = 0; i < aiscene->mNumMeshes; i++)
		{
			const aiMesh* aimesh = aiscene->mMeshes[i];

			aiMaterial *aiMat = nullptr;

			if (aimesh->mMaterialIndex >= 0)
				aiMat = aiscene->mMaterials[aimesh->mMaterialIndex];

			if (matPaths.size() > 0)
			{
				MeshMaterial mm;
				mm.mesh = ProcessAnimatedMesh(renderer, modelSerializer, am, aiscene, aimesh);

				const std::string &matName = matPaths[am->GetMeshesAndMaterials().size()];

				if (matName.size() > 0)
					mm.mat = renderer->CreateMaterialInstance(scriptManager, matName, mm.mesh.vao->GetVertexInputDescs());
				else
					mm.mat = LoadMaterialFromAssimpMat(renderer, &scriptManager, path, mm.mesh, aiMat, true);

				if (aiMat)
				{
					aiString name;
					aiMat->Get(AI_MATKEY_NAME, name);
					strncpy(mm.mat->name, name.C_Str(), 64);
				}

				am->AddMeshMaterial(mm);
			}
			else
			{
				MeshMaterial mm;
				mm.mesh = ProcessAnimatedMesh(renderer, modelSerializer, am, aiscene, aimesh);
				mm.mat = LoadMaterialFromAssimpMat(renderer, &scriptManager, path, mm.mesh, aiMat, true);

				if (aiMat)
				{
					aiString name;
					aiMat->Get(AI_MATKEY_NAME, name);
					strncpy(mm.mat->name, name.C_Str(), 64);
				}

				am->AddMeshMaterial(mm);
			}
		}

		Bone *rootBone = am->GetRootBone();
		rootBone->parent = nullptr;
		rootBone->name = std::string(aiscene->mRootNode->mName.data);

		glm::mat4 nodeTransformation;
		nodeTransformation[0] = glm::vec4(aim.a1, aim.b1, aim.c1, aim.d1);
		nodeTransformation[1] = glm::vec4(aim.a2, aim.b2, aim.c2, aim.d2);
		nodeTransformation[2] = glm::vec4(aim.a3, aim.b3, aim.c3, aim.d3);
		nodeTransformation[3] = glm::vec4(aim.a3, aim.b4, aim.c4, aim.d4);

		rootBone->transformation = nodeTransformation;

		for (size_t i = 0; i < aiscene->mRootNode->mNumChildren; i++)
		{
			BuildBoneTree(aiscene->mRootNode->mChildren[i], rootBone);
		}



		newModelPath += "model";

		am->SetPath(newModelPath);

		// Save all the bone related info
		const std::vector<glm::mat4> &boneOffsetMatrices = am->GetBoneOffsetMatrices();
		const std::map<std::string, unsigned int> &boneMap = am->GetBoneMap();

		modelSerializer.Write((unsigned int)boneOffsetMatrices.size());
		modelSerializer.Write(boneOffsetMatrices.data(), (unsigned int)boneOffsetMatrices.size() * sizeof(glm::mat4));

		for (auto it = boneMap.begin(); it != boneMap.end(); it++)
		{
			modelSerializer.Write(it->first);
			modelSerializer.Write(it->second);
		}

		// Save the bone tree
		SerializeBoneTree(modelSerializer, rootBone);

		modelSerializer.Save(newModelPath);
		modelSerializer.Close();

		delete am;
	}

	Mesh AssimpLoader::ProcessMesh(Renderer *renderer, Serializer &s, Model *model, const aiScene *aiscene, const aiMesh *aimesh, bool willUseInstancing, bool loadVertexColors)
	{
		std::vector<unsigned short> indices;

		for (unsigned int i = 0; i < aimesh->mNumFaces; i++)
		{
			aiFace face = aimesh->mFaces[i];

			for (unsigned int j = 0; j < face.mNumIndices; j++)
			{
				indices.push_back(face.mIndices[j]);
			}
		}

		s.Write(aimesh->mNumVertices);
		s.Write((unsigned int)indices.size());


		if (loadVertexColors)
		{
			std::vector<VertexPOS3D_UV_NORMAL_COLOR> vertices(aimesh->mNumVertices);

			for (unsigned int i = 0; i < aimesh->mNumVertices; i++)
			{
				VertexPOS3D_UV_NORMAL_COLOR &v = vertices[i];

				v.pos = glm::vec3(aimesh->mVertices[i].x, aimesh->mVertices[i].y, aimesh->mVertices[i].z);
				v.normal = glm::vec3(aimesh->mNormals[i].x, aimesh->mNormals[i].y, aimesh->mNormals[i].z);

				if (aimesh->HasVertexColors(0))
					v.color = glm::vec3(aimesh->mColors[0][i].r, aimesh->mColors[0][i].g, aimesh->mColors[0][i].b);
				else
					v.color = glm::vec3(0.0f);


				const AABB &originalAABB = model->GetOriginalAABB();
				model->SetOriginalAABB({ glm::min(originalAABB.min, v.pos),glm::max(originalAABB.max, v.pos) });

				if (aimesh->mTextureCoords[0])
				{
					// A vertex can contain up to 8 different texture coordinates. We thus make the assumption that we won't
					// use models where a vertex can have multiple texture coordinates so we always take the first set (0).
					v.uv = glm::vec2(aimesh->mTextureCoords[0][i].x, aimesh->mTextureCoords[0][i].y);
				}
				else
				{
					v.uv = glm::vec2(0.0f, 0.0f);
				}
			}

			Buffer *vb = renderer->CreateVertexBuffer(vertices.data(), vertices.size() * sizeof(VertexPOS3D_UV_NORMAL_COLOR), BufferUsage::STATIC);
			Buffer *ib = renderer->CreateIndexBuffer(indices.data(), indices.size() * sizeof(unsigned short), BufferUsage::STATIC);

			Mesh m = {};
			m.vertexOffset = 0;
			m.indexCount = indices.size();
			m.indexOffset = 0;
			m.instanceCount = 0;
			m.instanceOffset = 0;

			VertexAttribute attribs[4] = {};
			attribs[0].count = 3;						// Position
			attribs[1].count = 2;						// UV
			attribs[2].count = 3;						// Normal
			attribs[3].count = 3;						// Color

			attribs[0].offset = 0;
			attribs[1].offset = 3 * sizeof(float);
			attribs[2].offset = 5 * sizeof(float);
			attribs[3].offset = 8 * sizeof(float);

			VertexInputDesc desc = {};
			desc.stride = sizeof(VertexPOS3D_UV_NORMAL_COLOR);
			desc.attribs = { attribs[0], attribs[1], attribs[2], attribs[3] };

			if (willUseInstancing)
			{
				// Add the model matri attrib
				VertexAttribute instAttribs[4] = {};		// One mat4 needs 4 vec4
				instAttribs[0].count = 4;
				instAttribs[1].count = 4;
				instAttribs[2].count = 4;
				instAttribs[3].count = 4;

				instAttribs[0].offset = 0;
				instAttribs[1].offset = 4 * sizeof(float);
				instAttribs[2].offset = 8 * sizeof(float);
				instAttribs[3].offset = 12 * sizeof(float);

				VertexInputDesc instDesc = {};
				instDesc.stride = 16 * sizeof(float);
				instDesc.attribs = { instAttribs[0], instAttribs[1], instAttribs[2], instAttribs[3] };
				instDesc.instanced = true;

				VertexInputDesc descs[2] = { desc,instDesc };

				m.vao = renderer->CreateVertexArray(descs, 2, { vb }, ib);

			}
			else
			{
				m.vao = renderer->CreateVertexArray(&desc, 1, { vb }, ib);
			}

			// Prevent the min and max from being both 0 (when loading a plane for example)
			AABB originalAABB = model->GetOriginalAABB();

			if (originalAABB.min.y < 0.001f && originalAABB.min.y > -0.001f)
				originalAABB.min.y = -0.01f;
			if (originalAABB.max.y < 0.001f && originalAABB.max.y > -0.001f)
				originalAABB.max.y = 0.01f;

			model->SetOriginalAABB(originalAABB);

			return m;
		}
		else
		{
			std::vector<VertexPOS3D_UV_NORMAL> vertices(aimesh->mNumVertices);

			for (unsigned int i = 0; i < aimesh->mNumVertices; i++)
			{
				VertexPOS3D_UV_NORMAL &v = vertices[i];

				v.pos = glm::vec3(aimesh->mVertices[i].x, aimesh->mVertices[i].y, aimesh->mVertices[i].z);
				v.normal = glm::vec3(aimesh->mNormals[i].x, aimesh->mNormals[i].y, aimesh->mNormals[i].z);

				const AABB &originalAABB = model->GetOriginalAABB();
				model->SetOriginalAABB({ glm::min(originalAABB.min, v.pos),glm::max(originalAABB.max, v.pos) });

				if (aimesh->mTextureCoords[0])
				{
					// A vertex can contain up to 8 different texture coordinates. We thus make the assumption that we won't
					// use models where a vertex can have multiple texture coordinates so we always take the first set (0).
					v.uv = glm::vec2(aimesh->mTextureCoords[0][i].x, aimesh->mTextureCoords[0][i].y);
				}
				else
				{
					v.uv = glm::vec2(0.0f, 0.0f);
				}
			}

			Buffer *vb = renderer->CreateVertexBuffer(vertices.data(), vertices.size() * sizeof(VertexPOS3D_UV_NORMAL), BufferUsage::STATIC);
			Buffer *ib = renderer->CreateIndexBuffer(indices.data(), indices.size() * sizeof(unsigned short), BufferUsage::STATIC);

			Mesh m = {};
			m.vertexOffset = 0;
			m.indexCount = indices.size();
			m.indexOffset = 0;
			m.instanceCount = 0;
			m.instanceOffset = 0;

			VertexAttribute attribs[3] = {};
			attribs[0].count = 3;						// Position
			attribs[1].count = 2;						// UV
			attribs[2].count = 3;						// Normal

			attribs[0].offset = 0;
			attribs[1].offset = 3 * sizeof(float);
			attribs[2].offset = 5 * sizeof(float);

			VertexInputDesc desc = {};
			desc.stride = sizeof(VertexPOS3D_UV_NORMAL);
			desc.attribs = { attribs[0], attribs[1], attribs[2] };

			if (willUseInstancing)
			{
				// Add the model matri attrib
				VertexAttribute instAttribs[4] = {};		// One mat4 needs 4 vec4
				instAttribs[0].count = 4;
				instAttribs[1].count = 4;
				instAttribs[2].count = 4;
				instAttribs[3].count = 4;

				instAttribs[0].offset = 0;
				instAttribs[1].offset = 4 * sizeof(float);
				instAttribs[2].offset = 8 * sizeof(float);
				instAttribs[3].offset = 12 * sizeof(float);

				VertexInputDesc instDesc = {};
				instDesc.stride = 16 * sizeof(float);
				instDesc.attribs = { instAttribs[0], instAttribs[1], instAttribs[2], instAttribs[3] };
				instDesc.instanced = true;

				VertexInputDesc descs[2] = { desc,instDesc };

				m.vao = renderer->CreateVertexArray(descs, 2, { vb }, ib);
			}
			else
			{
				m.vao = renderer->CreateVertexArray(&desc, 1, { vb }, ib);
			}

			// Prevent the min and max from being both 0 (when loading a plane for example)
			AABB originalAABB = model->GetOriginalAABB();

			if (originalAABB.min.y < 0.001f && originalAABB.min.y > -0.001f)
				originalAABB.min.y = -0.01f;
			if (originalAABB.max.y < 0.001f && originalAABB.max.y > -0.001f)
				originalAABB.max.y = 0.01f;

			model->SetOriginalAABB(originalAABB);


			s.Write(vertices.data(), (unsigned int)vertices.size() * sizeof(VertexPOS3D_UV_NORMAL));
			s.Write(indices.data(), (unsigned int)indices.size() * sizeof(unsigned short));

			return m;
		}
	}

	Mesh AssimpLoader::ProcessAnimatedMesh(Renderer *renderer, Serializer &s, AnimatedModel *am, const aiScene *aiscene, const aiMesh *aimesh)
	{
		std::vector<unsigned short> indices;

		for (unsigned int i = 0; i < aimesh->mNumFaces; i++)
		{
			aiFace face = aimesh->mFaces[i];

			for (unsigned int j = 0; j < face.mNumIndices; j++)
			{
				indices.push_back(face.mIndices[j]);
			}
		}

		s.Write(aimesh->mNumVertices);
		s.Write((unsigned int)indices.size());

		std::vector<VertexPOS3D_UV_NORMAL_BONES> vertices(aimesh->mNumVertices);

		for (unsigned int i = 0; i < aimesh->mNumVertices; i++)
		{
			VertexPOS3D_UV_NORMAL_BONES v = {};

			v.pos = glm::vec3(aimesh->mVertices[i].x, aimesh->mVertices[i].y, aimesh->mVertices[i].z);
			v.normal = glm::vec3(aimesh->mNormals[i].x, aimesh->mNormals[i].y, aimesh->mNormals[i].z);

			const AABB &originalAABB = am->GetOriginalAABB();
			am->SetOriginalAABB({ glm::min(originalAABB.min, v.pos),glm::max(originalAABB.max, v.pos) });

			if (aimesh->mTextureCoords[0])
				v.uv = glm::vec2(aimesh->mTextureCoords[0][i].x, aimesh->mTextureCoords[0][i].y);
			else
				v.uv = glm::vec2(0.0f, 0.0f);

			v.weights = glm::vec4(0.0f);

			vertices[i] = v;
		}

		std::map<std::string, unsigned int> &boneMap = am->GetBoneMap();
		unsigned int numBones = boneMap.size();				// Start/Continue at current number of bones, otherwise for the next mesh we would start at 0 again and would mess up the boneMap indices

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

				am->AddBoneOffsetMatrix(boneOffset);
				am->AddBoneTransform(glm::mat4(1.0f));

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
		AABB originalAABB = am->GetOriginalAABB();

		if (originalAABB.min.y < 0.001f && originalAABB.min.y > -0.001f)
			originalAABB.min.y = -0.01f;
		if (originalAABB.max.y < 0.001f && originalAABB.max.y > -0.001f)
			originalAABB.max.y = 0.01f;

		am->SetOriginalAABB(originalAABB);

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

		s.Write(vertices.data(), (unsigned int)vertices.size() * sizeof(VertexPOS3D_UV_NORMAL_BONES));
		s.Write(indices.data(), (unsigned int)indices.size() * sizeof(unsigned short));

		return m;
	}

	MaterialInstance *AssimpLoader::LoadMaterialFromAssimpMat(Renderer *renderer, ScriptManager *scriptManager, const std::string &modelPath, const Mesh &mesh, const aiMaterial *aimat, bool isAnimated)
	{
		MaterialInstance *mat = nullptr;

		Log::Print(LogLevel::LEVEL_INFO, "Loading material\n");

		if (aimat)
		{
			unsigned int count = aimat->GetTextureCount(aiTextureType_DIFFUSE);

			Log::Print(LogLevel::LEVEL_INFO, "Num Textures: %u\n", count);

			if (count > 0)
			{
				// Get the first texture of this type. We don't support more than one texture per type
				aiString str;
				aimat->GetTexture(aiTextureType_DIFFUSE, 0, &str);

				std::string filename = std::string(str.C_Str());
				std::string directory = modelPath.substr(0, modelPath.find_last_of('/'));

				TextureParams params = {};
				params.filter = TextureFilter::LINEAR;
				params.format = TextureFormat::RGBA;
				params.internalFormat = TextureInternalFormat::SRGB8_ALPHA8;
				params.type = TextureDataType::UNSIGNED_BYTE;
				params.useMipmapping = true;
				params.wrap = TextureWrap::REPEAT;

				if (isAnimated)
					mat = renderer->CreateMaterialInstance(*scriptManager, "Data/Materials/modelDefaultAnimated.mat", mesh.vao->GetVertexInputDescs());		// use CreateMaterialInstance otherwise the ANIMATED define isn't set
				else
					mat = renderer->CreateMaterialInstanceFromBaseMat(*scriptManager, "Data/Materials/model_mat.lua", mesh.vao->GetVertexInputDescs());

				std::string path = directory + '/' + filename;

				if (utils::DirectoryExists(path))
					mat->textures[0] = renderer->CreateTexture2D(directory + '/' + filename, params);

				if (mat->textures[0] == nullptr)
				{
					mat->textures[0] = renderer->CreateTexture2D("Data/Textures/white.png", params);
				}
				renderer->UpdateMaterialInstance(mat);
			}
			else
			{
				if (isAnimated)
					mat = renderer->CreateMaterialInstance(*scriptManager, "Data/Materials/modelDefaultAnimated.mat", mesh.vao->GetVertexInputDescs());
				else
					mat = renderer->CreateMaterialInstance(*scriptManager, "Data/Materials/modelDefault.mat", mesh.vao->GetVertexInputDescs());
			}
		}
		else
		{
			if (isAnimated)
				mat = renderer->CreateMaterialInstance(*scriptManager, "Data/Materials/modelDefaultAnimated.mat", mesh.vao->GetVertexInputDescs());
			else
				mat = renderer->CreateMaterialInstance(*scriptManager, "Data/Materials/modelDefault.mat", mesh.vao->GetVertexInputDescs());
		}

		Log::Print(LogLevel::LEVEL_INFO, "Done loading material\n");

		return mat;
	}

	void AssimpLoader::BuildBoneTree(aiNode *node, Bone *parent)
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

	void AssimpLoader::StoreAnimation(Animation *a, aiAnimation *anim)
	{
		a->name = std::string(anim->mName.data);
		if (a->name.empty())
			a->name = "Animation";

		a->duration = static_cast<float>(anim->mDuration);
		a->ticksPerSecond = (float)(anim->mTicksPerSecond != 0.0 ? anim->mTicksPerSecond : 25.0f);

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

			a->bonesKeyframesList.insert({ std::string(bone->mNodeName.data), list });
		}
	}

	void AssimpLoader::SerializeBoneTree(Serializer &s, Bone *bone)
	{
		s.Write(bone->name);
		s.Write(bone->transformation);
		s.Write((unsigned int)bone->children.size());
		for (size_t i = 0; i < bone->children.size(); i++)
		{
			SerializeBoneTree(s, bone->children[i]);
		}
	}

	void AssimpLoader::SerializeAnimation(FileManager *fileManager, Animation *a)
	{
		Serializer s(fileManager);
		s.OpenForWriting();

		s.Write(315);
		s.Write(a->name);
		s.Write(a->duration);
		s.Write(a->ticksPerSecond);
		s.Write((unsigned int)a->bonesKeyframesList.size());

		for (auto it = a->bonesKeyframesList.begin(); it != a->bonesKeyframesList.end(); it++)
		{
			s.Write(it->first);
			s.Write((unsigned int)it->second.size());
			s.Write(it->second.data(), (unsigned int)it->second.size() * sizeof(Keyframe));
		}

		s.Save(a->path);
		s.Close();
	}

	void AssimpLoader::LoadSeparateAnimation(FileManager *fileManager, const std::string &path, const std::string &newAnimPath)
	{
		// To add an animation from a separate file to this model we will just load the file and grab all the animation bone info
		Assimp::Importer importer;
		const aiScene* tempScene = importer.ReadFile(path, 0);

		if (!tempScene || !tempScene->mRootNode)
		{
			Log::Print(LogLevel::LEVEL_ERROR, "Failed to load animation file\n");
			Log::Print(LogLevel::LEVEL_ERROR, "Assimp Error: %s\n", importer.GetErrorString());
			return;
		}

		if (tempScene->mNumAnimations == 0)
		{
			Log::Print(LogLevel::LEVEL_ERROR, "Animation file   %s   does not have any animation\n");
			return;
		}

		aiAnimation *anim = tempScene->mAnimations[0];

		std::string name = std::string(anim->mName.data);
		if (name.empty())
			name = "Animation";

		std::map<std::string, KeyFrameList> bonesKeyframesList;

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

		Serializer s(fileManager);
		s.OpenForWriting();

		s.Write(315);
		s.Write(name);
		s.Write(static_cast<float>(anim->mDuration));
		s.Write((float)(anim->mTicksPerSecond != 0.0 ? anim->mTicksPerSecond : 25.0f));
		s.Write((unsigned int)bonesKeyframesList.size());

		for (auto it = bonesKeyframesList.begin(); it != bonesKeyframesList.end(); it++)
		{
			s.Write(it->first);
			s.Write((unsigned int)it->second.size());
			s.Write(it->second.data(), (unsigned int)it->second.size() * sizeof(Keyframe));
		}

		s.Save(newAnimPath);
		s.Close();
	}
}
