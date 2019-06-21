#include "Model.h"

#include "VertexTypes.h"
#include "ResourcesLoader.h"
#include "Material.h"
#include "Shader.h"
#include "VertexArray.h"
#include "Buffers.h"
#include "Game/Game.h"
#include "Program/FileManager.h"
#include "Program/Log.h"

#include "Program/Utils.h"

#include "include/assimp/Importer.hpp"
#include "include/assimp/postprocess.h"

#ifdef VITA
#include "psp2/kernel/sysmem.h"
#endif

namespace Engine
{
	Model::Model()
	{
		AddReference();
		type = ModelType::BASIC;
		isInstanced = false;
		castShadows = false;
		originalAABB.min = glm::vec3();
		originalAABB.max = glm::vec3();
		lodDistance = 10000.0f;
	}

	Model::Model(Renderer *renderer, const std::string &path, bool isInstanced, ScriptManager &scriptManager, bool loadVertexColors)
	{
		AddReference();
		type = ModelType::BASIC;	

		this->path = path;
		this->isInstanced = isInstanced;
		castShadows = true;
		lodDistance = 10000.0f;

		Log::Print(LogLevel::LEVEL_INFO, "Loading model at %s\n", this->path.c_str());

		LoadAssimpModelFile(renderer, {}, scriptManager, loadVertexColors);
	}

	Model::Model(Renderer *renderer, const std::string &path, bool isInstanced, const std::vector<std::string> &matNames, ScriptManager &scriptManager, bool loadVertexColors)
	{
		AddReference();
		type = ModelType::BASIC;

		this->path = path;
		this->isInstanced = isInstanced;
		castShadows = true;
		lodDistance = 10000.0f;
		
		LoadAssimpModelFile(renderer, matNames, scriptManager, loadVertexColors);
	}

	Model::Model(Renderer *renderer, const Mesh &mesh, MaterialInstance *mat, const AABB &aabb)
	{
		AddReference();
		type = ModelType::BASIC;

		isInstanced = false;
		castShadows = true;
		originalAABB = aabb;
		lodDistance = 10000.0f;

		meshesAndMaterials.push_back({ mesh,mat });
	}

	Model::~Model()
	{
		for (size_t i = 0; i < meshesAndMaterials.size(); i++)
		{
			MeshMaterial &meshMat = meshesAndMaterials[i];

			if (meshMat.mesh.vao)
				delete meshMat.mesh.vao;

			for (size_t j = 0; j < meshMat.mat->textures.size(); j++)
			{
				if (meshMat.mat->textures[j])
					meshMat.mat->textures[j]->RemoveReference();
			}
		}
	}

	void Model::LoadAssimpModelFile(Renderer *renderer, const std::vector<std::string> &matNames, ScriptManager &scriptManager, bool loadVertexColors)
	{
		originalAABB.min = glm::vec3(100000.0f);
		originalAABB.max = glm::vec3(-100000.0f);

		Assimp::Importer importer;

/*#ifdef VITA
		const aiScene* scene = importer.ReadFile(renderer->GetFileManager()->GetAppPath() + path, aiProcess_JoinIdenticalVertices | aiProcess_FlipUVs); //| aiProcess_GenSmoothNormals); //| aiProcess_CalcTangentSpace);
		SceKernelFreeMemorySizeInfo info = {};
		info.size = sizeof(SceKernelFreeMemorySizeInfo);
		sceKernelGetFreeMemorySize(&info);
		Log::Print(LogLevel::LEVEL_INFO, "USER_CDRAM_RW memory available size: %d\n", info.size_cdram);
		Log::Print(LogLevel::LEVEL_INFO, "USER_MAIN_PHYCONT_*_RW available size: %d\n", info.size_phycont);
		Log::Print(LogLevel::LEVEL_INFO, "USER_RW available size: %d\n", info.size_user);
#else*/
		const aiScene* scene = importer.ReadFile(path, aiProcess_JoinIdenticalVertices | aiProcess_FlipUVs); //| aiProcess_GenSmoothNormals); //| aiProcess_CalcTangentSpace);
//#endif

		if (!scene || scene->mFlags == AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
		{
			Log::Print(LogLevel::LEVEL_ERROR, "Assimp Error: %s\n", importer.GetErrorString());
			return;
		}

		Log::Print(LogLevel::LEVEL_INFO, "Model imported\n");
		Log::Print(LogLevel::LEVEL_INFO, "Num Meshes: %d\n", scene->mNumMeshes);

		Serializer s(renderer->GetFileManager());
		s.OpenForWriting();

		s.Write(313);
		s.Write(scene->mNumMeshes);
		s.Write(2);						// Vertex type

		// Load all the model meshes
		for (unsigned int i = 0; i < scene->mNumMeshes; i++)
		{
			const aiMesh* aiMesh = scene->mMeshes[i];

			aiMaterial *aiMat = nullptr;

			if (aiMesh->mMaterialIndex >= 0)			
				aiMat = scene->mMaterials[aiMesh->mMaterialIndex];

			if (matNames.size() > 0)
			{
				MeshMaterial mm;
				mm.mesh = ProcessMesh(renderer, s, aiMesh, scene, loadVertexColors);

				const std::string &matName = matNames[meshesAndMaterials.size()];

				if (matName.size() > 0)
					mm.mat = renderer->CreateMaterialInstance(scriptManager, matName, mm.mesh.vao->GetVertexInputDescs());
				else
					mm.mat = LoadMaterialFromAssimpMat(renderer, scriptManager, mm.mesh, aiMat);

				if (aiMat)
				{
					aiString name;
					aiMat->Get(AI_MATKEY_NAME, name);
					strncpy(mm.mat->name, name.C_Str(), 64);
				}

				meshesAndMaterials.push_back(mm);
			}
			else
			{
				// Workaround for terrain vegetation to work when adding through the editor with no materials
				if (isInstanced == false)
				{
					MeshMaterial mm;
					mm.mesh = ProcessMesh(renderer, s, aiMesh, scene, loadVertexColors);
					mm.mat = LoadMaterialFromAssimpMat(renderer, scriptManager, mm.mesh, aiMat);

					if (aiMat)
					{
						aiString name;
						aiMat->Get(AI_MATKEY_NAME, name);
						strncpy(mm.mat->name, name.C_Str(), 64);
					}

					meshesAndMaterials.push_back(mm);																											
				}
				else
				{
					MeshMaterial mm;
					mm.mesh = ProcessMesh(renderer, s, aiMesh, scene, loadVertexColors);
					mm.mat = renderer->CreateMaterialInstance(scriptManager, "Data/Materials/modelDefaultInstanced.mat", mm.mesh.vao->GetVertexInputDescs());

					if (aiMat)
					{
						aiString name;
						aiMat->Get(AI_MATKEY_NAME, name);
						strncpy(mm.mat->name, name.C_Str(), 64);
					}

					meshesAndMaterials.push_back(mm);
				}
			}
		}

		// Remove the model file extension and add our own so when we load the project again, the model will be loaded from our own custom format
		size_t dotIndex = path.find('.', 0) + 1;		// +1 to exclude the dot
		size_t extensionSize = path.length() - dotIndex;
		for (size_t i = 0; i < extensionSize; i++)
		{
			path.pop_back();
		}

		path += "model";

		s.Save(path);
		s.Close();
	}

	void Model::LoadModel(Renderer *renderer, ScriptManager &scriptManager, const std::vector<std::string> &matNames)
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

		if (magic != 313)
		{
			Log::Print(LogLevel::LEVEL_ERROR, "ERROR -> Unknown model file: %s\n", path.c_str());
			return;
		}

		unsigned int numMeshes = 0;
		int vertexType = 0;
		s.Read(numMeshes);
		s.Read(vertexType);

		Log::Print(LogLevel::LEVEL_INFO, "num meshes: %u\n", numMeshes);
		Log::Print(LogLevel::LEVEL_INFO, "vertex type: %d\n", vertexType);

		assert((size_t)numMeshes == matNames.size());

		meshesAndMaterials.resize((size_t)numMeshes);

		for (size_t i = 0; i < meshesAndMaterials.size(); i++)
		{
			unsigned int numVertices = 0;
			unsigned int numIndices = 0;
			s.Read(numVertices);
			s.Read(numIndices);

			std::vector<VertexPOS3D_UV_NORMAL> vertices(numVertices);
			std::vector<unsigned short> indices(numIndices);
			s.Read(vertices.data(), (unsigned int)vertices.size() * sizeof(VertexPOS3D_UV_NORMAL));
			s.Read(indices.data(), (unsigned int)indices.size() * sizeof(unsigned short));

			Log::Print(LogLevel::LEVEL_INFO, "Creating buffers\n");

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

			m.vao = renderer->CreateVertexArray(&desc, 1, { vb }, ib);

			MeshMaterial mm = {};
			mm.mat = renderer->CreateMaterialInstance(scriptManager, matNames[i], {desc});
			mm.mesh = m;

			std::string name = matNames[i].substr(matNames[i].find_last_of('/') + 1);;
			// Remove the extension
			name.pop_back();
			name.pop_back();
			name.pop_back();
			name.pop_back();

			strncpy(mm.mat->name, name.c_str(), 64);

			meshesAndMaterials[i] = mm;

			Log::Print(LogLevel::LEVEL_INFO, "Mesh loaded\n");
		}

		s.Close();
	}

	Mesh Model::ProcessMesh(Renderer *renderer, Serializer &s, const aiMesh *aimesh, const aiScene *aiscene, bool loadVertexColors)
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

				originalAABB.min = glm::min(originalAABB.min, v.pos);
				originalAABB.max = glm::max(originalAABB.max, v.pos);

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

			if (isInstanced)
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
			if (originalAABB.min.y < 0.001f && originalAABB.min.y > -0.001f)
				originalAABB.min.y = -0.01f;
			if (originalAABB.max.y < 0.001f && originalAABB.max.y > -0.001f)
				originalAABB.max.y = 0.01f;

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

				originalAABB.min = glm::min(originalAABB.min, v.pos);
				originalAABB.max = glm::max(originalAABB.max, v.pos);

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

			if (isInstanced)
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
			if (originalAABB.min.y < 0.001f && originalAABB.min.y > -0.001f)
				originalAABB.min.y = -0.01f;
			if (originalAABB.max.y < 0.001f && originalAABB.max.y > -0.001f)
				originalAABB.max.y = 0.01f;


			s.Write(vertices.data(), (unsigned int)vertices.size() * sizeof(VertexPOS3D_UV_NORMAL));
			s.Write(indices.data(), (unsigned int)indices.size() * sizeof(unsigned short));

			return m;
		}
	}

	MaterialInstance *Model::LoadMaterialFromAssimpMat(Renderer *renderer, ScriptManager &scriptManager, const Mesh &mesh, const aiMaterial *aiMat)
	{
		MaterialInstance *mat = nullptr;

		Log::Print(LogLevel::LEVEL_INFO, "Load material\n");

		if (aiMat)
		{
			unsigned int count = aiMat->GetTextureCount(aiTextureType_DIFFUSE);

			Log::Print(LogLevel::LEVEL_INFO, "Num Textures: %u\n", count);

			if (count > 0)
			{
				// Get the first texture of this type. We don't support more than one texture per type
				aiString str;
				aiMat->GetTexture(aiTextureType_DIFFUSE, 0, &str);

				std::string filename = std::string(str.C_Str());
				std::string directory = path.substr(0, path.find_last_of('/'));

				TextureParams params = {};
				params.filter = TextureFilter::LINEAR;
				params.format = TextureFormat::RGBA;
				params.internalFormat = TextureInternalFormat::SRGB8_ALPHA8;
				params.type = TextureDataType::UNSIGNED_BYTE;
				params.useMipmapping = true;
				params.wrap = TextureWrap::REPEAT;

				mat = renderer->CreateMaterialInstanceFromBaseMat(scriptManager, "Data/Materials/model_mat.lua", mesh.vao->GetVertexInputDescs());
				mat->textures[0] = renderer->CreateTexture2D(directory + '/' + filename, params);

				if (mat->textures[0] == nullptr)
				{
					mat->textures[0] = renderer->CreateTexture2D("Data/Textures/white.png", params);
				}
				renderer->UpdateMaterialInstance(mat);
			}
			else
			{
				if (type == ModelType::ANIMATED)
					mat = renderer->CreateMaterialInstance(scriptManager, "Data/Materials/modelDefaultAnimated.mat", mesh.vao->GetVertexInputDescs());
				else
					mat = renderer->CreateMaterialInstance(scriptManager, "Data/Materials/modelDefault.mat", mesh.vao->GetVertexInputDescs());
			}
		}
		else
		{
			if (type == ModelType::ANIMATED)
				mat = renderer->CreateMaterialInstance(scriptManager, "Data/Materials/modelDefaultAnimated.mat", mesh.vao->GetVertexInputDescs());
			else
				mat = renderer->CreateMaterialInstance(scriptManager, "Data/Materials/modelDefault.mat", mesh.vao->GetVertexInputDescs());
		}

		Log::Print(LogLevel::LEVEL_INFO, "Done loading material\n");

		return mat;
	}

	void Model::UpdateInstanceInfo(unsigned int instanceCount, unsigned int instanceOffset)
	{
		for (size_t i = 0; i < meshesAndMaterials.size(); i++)
		{
			meshesAndMaterials[i].mesh.instanceCount = instanceCount;
			meshesAndMaterials[i].mesh.instanceOffset = instanceOffset;
		}
	}

	void Model::SetMeshMaterial(unsigned short meshID, MaterialInstance *matInstance)
	{
		if (meshID < 0 || meshID >= meshesAndMaterials.size() || matInstance == nullptr)
			return;

		MeshMaterial &mm = meshesAndMaterials[meshID];

		// Remove texture ref from old mat
		for (size_t i = 0; i < mm.mat->textures.size(); i++)
		{
			if (mm.mat->textures[i])
				mm.mat->textures[i]->RemoveReference();
		}

		mm.mat = matInstance;
	}

	MaterialInstance *Model::GetMaterialInstanceOfMesh(unsigned short meshID) const
	{
		if (meshID < 0 || meshID >= meshesAndMaterials.size())
			return nullptr;

		return meshesAndMaterials[meshID].mat;
	}

	void Model::Serialize(Serializer &s) const
	{
		s.Write(path);
		s.Write(isInstanced);
		s.Write(castShadows);
		s.Write(lodDistance);
		s.Write(originalAABB.min);
		s.Write(originalAABB.max);

		s.Write((unsigned int)meshesAndMaterials.size());

		for (size_t i = 0; i < meshesAndMaterials.size(); i++)
			s.Write(meshesAndMaterials[i].mat->path);
	}

	void Model::Deserialize(Serializer &s, Game *game, bool reload)
	{
		s.Read(path);
		s.Read(isInstanced);
		s.Read(castShadows);
		s.Read(lodDistance);
		s.Read(originalAABB.min);
		s.Read(originalAABB.max);

		unsigned int matCount;
		s.Read(matCount);

		std::vector<std::string> matNames(matCount);
		for (size_t i = 0; i < matCount; i++)
			s.Read(matNames[i]);

		if (!reload)
			//LoadAssimpModelFile(game->GetRenderer(), matNames, game->GetScriptManager(), false);
			LoadModel(game->GetRenderer(), game->GetScriptManager(), matNames);
	}
}
