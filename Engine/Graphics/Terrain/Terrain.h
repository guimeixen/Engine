#pragma once

#include "Graphics/Camera/Camera.h"
#include "TerrainNode.h"
#include "Graphics/RendererStructs.h"
#include "Graphics/Mesh.h"
#include "Game/EntityManager.h"

#include <string>

namespace Engine
{
	struct MaterialInstance;
	class Game;
	class Renderer;
	class Model;
	class Collider;
	class Buffer;

	enum class DeformType
	{
		RAISE,
		LOWER,
		FLATTEN,
		SMOOTH
	};

	struct TerrainInfo
	{
		std::string matPath;
		std::string vegPath;
	};

	struct TerrainMaterialUBO
	{
		glm::vec2 terrainParams;
		glm::vec3 selectionPointAndRadius;
	};

	struct VegetationCell
	{
		std::vector<Vegetation> vegTypes;
		std::vector<ModelInstanceData> data;
		AABB aabb;
		unsigned int x;
		unsigned int z;
	};

	struct VegColInfo
	{
		Collider *col;
		float dist2;
		glm::vec3 position;
	};

	class Terrain : public RenderQueueGenerator
	{
	public:
		Terrain();
		~Terrain();

		bool Init(Game *game, const TerrainInfo &terrainInfo);

		void Cull(unsigned int passAndFrustumCount, unsigned int *passIds, const Frustum *frustums, std::vector<VisibilityIndices*> &out) override;
		void GetRenderItems(unsigned int passCount, unsigned int *passIds, const VisibilityIndices &visibility, RenderQueue &outQueues) override;
		void UpdateLOD(Camera *camera);
		void UpdateVegColliders(Camera *camera);
		void Dispose();

		bool IsVisible() const { return data.size() > 0; }
		RenderItem GetTerrainRenderItem();

		void UpdateEditing();
		void EnableEditing();
		void DisableEditing();
		void DeformTerrain();
		bool IsEditable() const { return editingEnabled; }
		bool IsBeingEdited() const { return isBeingEdited; }

		void AddVegetation(const std::string &modelPath);
		void ChangeVegetationModel(Vegetation &v, int lod, const std::string &newModelPath);
		void PaintVegetation(const std::vector<int> &ids, const glm::vec3 &rayOrigin, const glm::vec3 &rayDir);
		void UndoVegetationPaint(const std::vector<int> &ids);
		void RedoVegetationPaint(const std::vector<int> &ids);
		void ReseatVegetation();
		void CreateVegColliders();
		void SetVegCollidersRefPoint(const glm::vec3 &point);

		void SetMaterial(const std::string &path);
		void SetHeightmap(const std::string &path);
		void SetHeightScale(float scale);
		void SetBrushRadius(float radius);
		void SetBrushStrength(float strength);
		void SetVegetationBrushRadius(float radius);

		bool IntersectTerrain(const glm::vec3 &rayOrigin, const glm::vec3 &rayDir, glm::vec3 &intersectionPoint);

		MaterialInstance *GetMaterialInstance() const { return matInstance; }

		int GetResolution() const { return resolution; }

		float GetHeightAt(int x, int z);
		float GetExactHeightAt(float x, float z);
		glm::vec3 GetNormalAtFast(int x, int z);
		glm::vec3 GetNormalAt(float x, float z);
		float GetHeightScale() const { return heightScale; }
		float GetBrushRadius() const { return brushRadius; }
		float GetBrushStrength() const { return brushStrength; }
		float GetVegetationBrushRadius() const { return vegBrushRadius; }
		const glm::vec3 &GetIntersectionPoint() const { return intersectionPoint; }

		std::vector<Vegetation> &GetVegetation() { return vegetation; }
		const std::vector<ModelInstanceData> &GetVegetationInstanceData() const { return vegetationInstData; }

		void Save(const std::string &folder, const std::string &sceneName);

	private:
		void SmoothTerrain();
		void CreateMesh();
		void CreateQuadtree();
		void CalculateLODVisRanges();
		void LoadVegetationFile(const std::string &vegPath);	
		bool InBounds(int x, int z);
		void CreateVegInstanceBuffer();
		void AddVegInstanceBufferToMesh(const Mesh &mesh, int lod);
		float Barycentric(const glm::vec3 &p1, const glm::vec3 &p2, const glm::vec3 &p3, const glm::vec2 &pos);

	private:
		Game *game;
		Renderer *renderer;
		static const int lodLevels = 4;
		int resolution;
		float heightScale = 1.0f;
		float *heights;
		Mesh mesh;
		MaterialInstance *matInstance;
		Buffer *terrainInstancingBuffer;
		int terrainShapeID = -1;

		bool editingEnabled = false;
		bool isBeingEdited = false;
		glm::vec3 intersectionPoint;
		DeformType deformType = DeformType::RAISE;
		float brushRadius = 10.0f;
		float brushStrength = 10.0f;
		float flattenHeight = 0.0f;

		unsigned int opaquePassID = 0;
		unsigned int csmPassID = 0;

		bool isTerrainDataUpdated = false;

		bool updateMaterialUBO = false;

		std::vector<TerrainInstanceData> data;

		std::vector<VertexInputDesc> terrainInputDescs;

		float lodFar = 640.0f;
		float lodVisRanges[lodLevels];
		const int meshSize = 16;			// If changing mesh size, the grid dim in the shader also needs to be changed

		float vegBrushRadius = 0.1f;

		std::vector<std::vector<TerrainNode*>> nodes;

		unsigned char *obstacleMap = nullptr;
		int obstacleMapWidth = 0;
		int obstacleMapHeight = 0;

		std::vector<Vegetation> vegetation;
		Buffer *vegInstancingBufferLOD0;
		Buffer *vegInstancingBufferLOD1;
		Buffer *vegInstancingBufferLOD2;

		std::vector<ModelInstanceData> vegetationInstData;
		std::vector<ModelInstanceData> culledVegInstDataLOD0;
		std::vector<ModelInstanceData> culledVegInstDataLOD1;
		std::vector<ModelInstanceData> culledVegInstDataLOD2;

		//std::vector<VegetationCell> vegCells;
		//unsigned int vegCellSize = 64;
		std::vector<VegColInfo> closestColliders;
		bool vegColCreated = false;
		unsigned int maxColliders = 20;
		glm::vec3 collidersRefPoint;
	};
}
