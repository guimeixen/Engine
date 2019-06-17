#pragma once

#include "Mesh.h"
#include "Physics/BoundingVolumes.h"
#include "Program/Serializer.h"
#include "Game/ComponentManagers/ScriptManager.h"

#include "include/assimp/scene.h"

#include <string>
#include <vector>

namespace Engine
{
	class Renderer;
	struct MaterialInstance;
	class Game;

	enum class ModelType
	{
		BASIC,
		ANIMATED
	};

	struct MeshMaterial
	{
		Mesh mesh;
		MaterialInstance *mat;
	};

	class Model
	{
	public:
		Model();
		Model(Renderer *renderer, const std::string &path, bool isInstanced, ScriptManager &scriptManager, bool loadVertexColors = false);
		Model(Renderer *renderer, const std::string &path, bool isInstanced, const std::vector<std::string> &matNames, ScriptManager &scriptManager, bool loadVertexColors = false);
		Model(Renderer *renderer, const Mesh &mesh, MaterialInstance *mat, const AABB &aabb);
		virtual ~Model();

		const std::vector<MeshMaterial> &GetMeshesAndMaterials() const { return meshesAndMaterials; }
		const AABB &GetOriginalAABB() const { return originalAABB; }
		const std::string &GetPath() const { return path; }
		ModelType GetType() const { return type; }

		void UpdateInstanceInfo(unsigned int instanceCount, unsigned int instanceOffset);
		void SetMeshMaterial(unsigned short meshID, MaterialInstance *matInstance);
		MaterialInstance *GetMaterialInstanceOfMesh(unsigned short meshID) const;

		void SetCastShadows(bool cast) { castShadows = cast; }
		bool GetCastShadows() const { return castShadows; }

		void AddInstanceData(const glm::mat4 &m) { instanceData.push_back(m); }
		void ClearInstanceData() { instanceData.clear(); }
		const glm::mat4 *GetInstanceData() const { return instanceData.data(); }
		unsigned int GetInstanceDataSize() const { return static_cast<unsigned int>(instanceData.size()); }

		void SetLODDistance(float distance) { lodDistance = distance; }
		float GetLODDistance() const { return lodDistance; }

		void Serialize(Serializer &s) const;
		void Deserialize(Serializer &s, Game *game, bool reload = false);

		void AddReference() { refCount++; }
		void RemoveReference() { refCount--; }
		unsigned int GetRefCount() const { return refCount; }

	protected:
		void LoadModelFile(Renderer *renderer, const std::vector<std::string> &matNames, ScriptManager &scriptManager, bool loadVertexColors);
		Mesh ProcessMesh(Renderer *renderer, const aiMesh *aimesh, const aiScene *aiscene, bool loadVertexColors);
		MaterialInstance *LoadMaterialFromAssimpMat(Renderer *renderer, ScriptManager &scriptManager, const Mesh &mesh, const aiMaterial *aiMat);

	protected:
		ModelType type;
		std::vector<MeshMaterial> meshesAndMaterials;
		std::string path;
		bool isInstanced;
		bool castShadows;
		float lodDistance;
		AABB originalAABB;
		std::vector<glm::mat4> instanceData;
		unsigned int refCount = 0;
	};
}
