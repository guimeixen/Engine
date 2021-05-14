#include "Model.h"

#include "VertexTypes.h"
#include "Material.h"
#include "VertexArray.h"
#include "Buffers.h"
#include "Game/Game.h"
#include "Program/FileManager.h"
#include "Program/Log.h"
#include "Program/Utils.h"

namespace Engine
{
	Model::Model()
	{
		AddReference();
		type = ModelType::BASIC;
		castShadows = true;
		originalAABB = { glm::vec3(100000.0f), glm::vec3(-100000.0f) };
		lodDistance = 10000.0f;
	}

	Model::Model(Renderer *renderer, ScriptManager &scriptManager, const std::string &path, const std::vector<std::string> &matNames)
	{
		AddReference();
		type = ModelType::BASIC;
		this->path = path;
		castShadows = true;
		originalAABB = { glm::vec3(100000.0f), glm::vec3(-100000.0f) };
		lodDistance = 10000.0f;

		LoadModel(renderer, scriptManager, matNames);
	}

	Model::Model(Renderer *renderer, const Mesh &mesh, MaterialInstance *mat, const AABB &aabb, ModelType type)
	{
		AddReference();
		this->type = type;

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

			// Remove reference because one vao could be shared between models, eg. various cube primivite models will al use the cube vao so we don't repeat vertex and index buffers
			if (meshMat.mesh.vao)
			{
				meshMat.mesh.vao->RemoveReference();
				meshMat.mesh.vao = nullptr;
			}

			for (size_t j = 0; j < meshMat.mat->textures.size(); j++)
			{
				if (meshMat.mat->textures[j])
					meshMat.mat->textures[j]->RemoveReference();
			}
		}
	}

	void Model::AddMeshMaterial(const MeshMaterial &mm)
	{
		meshesAndMaterials.push_back(mm);
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

		meshesAndMaterials.resize((size_t)numMeshes);

		for (size_t i = 0; i < meshesAndMaterials.size(); i++)
		{
			unsigned int numVertices = 0;
			unsigned int numIndices = 0;
			s.Read(numVertices);
			s.Read(numIndices);

			std::vector<VertexPOS3D_UV_NORMAL_TANGENT> vertices(numVertices);
			std::vector<unsigned short> indices(numIndices);
			s.Read(vertices.data(), (unsigned int)vertices.size() * sizeof(VertexPOS3D_UV_NORMAL_TANGENT));
			s.Read(indices.data(), (unsigned int)indices.size() * sizeof(unsigned short));

			Log::Print(LogLevel::LEVEL_INFO, "Creating buffers\n");

			Buffer *vb = renderer->CreateVertexBuffer(vertices.data(), vertices.size() * sizeof(VertexPOS3D_UV_NORMAL_TANGENT), BufferUsage::STATIC);
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
			attribs[3].count = 3;						// Tangent

			attribs[0].offset = 0;
			attribs[1].offset = 3 * sizeof(float);
			attribs[2].offset = 5 * sizeof(float);
			attribs[3].offset = 8 * sizeof(float);

			VertexInputDesc desc = {};
			desc.stride = sizeof(VertexPOS3D_UV_NORMAL_TANGENT);
			desc.attribs = { attribs[0], attribs[1], attribs[2], attribs[3] };

			m.vao = renderer->CreateVertexArray(&desc, 1, { vb }, ib);
			m.vao->AddReference();

			MeshMaterial mm = {};
			mm.mesh = m;

			std::string matName;

			if (matNames.size() == 0)
			{
				mm.mat = renderer->CreateMaterialInstance(scriptManager, "Data/Materials/modelDefault.mat", { desc });
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

		s.Close();
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
			LoadModel(game->GetRenderer(), game->GetScriptManager(), matNames);
	}
}
