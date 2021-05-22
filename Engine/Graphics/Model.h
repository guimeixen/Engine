#pragma once

#include "Mesh.h"
#include "Physics/BoundingVolumes.h"
#include "Game/ComponentManagers/ScriptManager.h"

struct aiMesh;
struct aiScene;
struct aiMaterial;

namespace Engine
{
	class Renderer;
	struct MaterialInstance;
	class Game;

	enum class ModelType
	{
		BASIC,
		ANIMATED,
		PRIMITIVE_CUBE,
		PRIMITIVE_SPHERE
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
		Model(Renderer *renderer, ScriptManager &scriptManager, const std::string &path, const std::vector<std::string> &matNames);
		Model(Renderer *renderer, const Mesh &mesh, MaterialInstance *mat, const AABB &aabb, ModelType type);
		virtual ~Model();

		void AddMeshMaterial(const MeshMaterial &mm);
		void AddInstanceData(const glm::mat4 &m) { instanceData.push_back(m); }
		void UpdateInstanceInfo(unsigned int instanceCount, unsigned int instanceOffset);
		void ClearInstanceData() { instanceData.clear(); }

		void SetPath(const std::string &path) { this->path = path; }
		void SetOriginalAABB(const AABB &aabb) { originalAABB = aabb; }
		void SetMeshMaterial(unsigned short meshID, MaterialInstance *matInstance);
		void SetCastShadows(bool cast) { castShadows = cast; }
		void SetLODDistance(float distance) { lodDistance = distance; }
	
		const std::vector<MeshMaterial> &GetMeshesAndMaterials() const { return meshesAndMaterials; }
		const AABB &GetOriginalAABB() const { return originalAABB; }
		const std::string &GetPath() const { return path; }
		bool GetCastShadows() const { return castShadows; }
		float GetLODDistance() const { return lodDistance; }
		const glm::mat4 *GetInstanceData() const { return instanceData.data(); }
		unsigned int GetInstanceDataSize() const { return static_cast<unsigned int>(instanceData.size()); }
		MaterialInstance *GetMaterialInstanceOfMesh(unsigned short meshID) const;
		ModelType GetType() const { return type; }

		void Serialize(Serializer &s) const;
		void Deserialize(Serializer &s, Game *game, bool reload = false);

		void AddReference() { refCount++; }
		void RemoveReference() { if (refCount > 1) { refCount--; } else { delete this; } }
		unsigned int GetRefCount() const { return refCount; }

	protected:
		void LoadModel(Renderer *renderer, ScriptManager &scriptManager, const std::vector<std::string> &matNames);

	protected:
		ModelType type;
		std::vector<MeshMaterial> meshesAndMaterials;
		std::string path;
		bool castShadows;
		float lodDistance;
		AABB originalAABB;
		std::vector<glm::mat4> instanceData;
		unsigned int refCount = 0;
	};
}
