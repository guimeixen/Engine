#pragma once

#include "Graphics/RendererStructs.h"
#include "Physics/BoundingVolumes.h"
#include "Graphics/Animation/AnimatedModel.h"

#include <unordered_map>
#include <map>

namespace Engine
{
	class Game;
	class TransformManager;
	class Camera;

	struct ModelInstance
	{
		Entity e;
		Model *model;
		//int type;
		//unsigned int index;
		AABB aabb;
	};

	class ModelManager : public RenderQueueGenerator
	{
	public:
		ModelManager() {}

		void Init(Game *game, unsigned int initialCapacity);
		void Update();
		void PartialDispose();
		void Dispose();

		void Cull(unsigned int passAndFrustumCount, unsigned int *passIds, const Frustum *frustums, std::vector<VisibilityIndices*> &out) override;
		void GetRenderItems(unsigned int passCount, unsigned int *passIds, const VisibilityIndices &visibility, RenderQueue &outQueues) override;

		Model *AddModel(Entity e, const std::string &path, bool animated = false);
		Model *AddModelFromMesh(Entity e, const Mesh &mesh, MaterialInstance *mat, const AABB &aabb);
		// ModelType Basic and Animated can't be used here
		Model *AddPrimitiveModel(Entity e, ModelType type);
		void DuplicateModel(Entity e, Entity newE);
		void RemoveModel(Entity e);
		Model *GetModel(Entity e) const;
		AnimatedModel *GetAnimatedModel(Entity e) const;
		const AABB &GetAABB(Entity e) const;

		bool HasModel(Entity e) const;
		bool HasAnimatedModel(Entity e) const;

		bool PerformRaycast(Camera *camera, const glm::vec2 &point, Entity &outEntity);
		
		Animation *LoadAnimation(const std::string &path);

		void AddAnimation(Animation *anim, const std::string &path);
		void RemoveModelNoEntity(Model *model);

		const std::map<unsigned int, Model*> &GetUniqueModels() const { return uniqueModels; }
		const std::map<unsigned int, Animation*> &GetAnimations() const { return animations; }
		const std::vector<ModelInstance> &GetModels() const { return models; }

		void Serialize(Serializer &s);
		void Deserialize(Serializer &s, bool reload = false);

	private:
		Model *LoadModel(const std::string &path, const std::vector<std::string> &matNames, bool isAnimated, bool isInstanced = false, bool loadVertexColors = false);
		Model *LoadModel(const Mesh &mesh, MaterialInstance *mat, const AABB &aabb);
		Mesh LoadPrimitive(ModelType type);
		//void LoadModelNew(unsigned int index, const std::string &path, const std::vector<std::string> &matNames, bool isInstanced = false, bool loadVertexColors = false);
		//Mesh ProcessMesh(unsigned int index, const aiMesh *aimesh, const aiScene *aiscene, bool isInstanced, bool loadVertexColors);

	private:
		struct ModelS
		{
			std::vector<MeshMaterial> meshes;
		};

		struct ModelsData
		{
			unsigned int size;
			unsigned int capacity;
			unsigned char *buffer;

			Entity *e;
			ModelS *model;
			unsigned int *pathIndex;
			bool *castShadows;
			float *lodDistance;
			AABB *originalAABB;
			AABB *worldSpaceAABB;
		};

		Game *game;
		TransformManager *transformManager;
		std::vector<ModelInstance> models;
		std::unordered_map<unsigned int, unsigned int> map;
		unsigned int usedModels = 0;

		ModelsData data;
		//std::vector<std::string> paths;

		std::map<unsigned int, Model*> uniqueModels;
		std::vector<AnimatedModel*> animatedModels;
		std::map<unsigned int, Animation*> animations;

		unsigned int shadowPassID;

		unsigned int modelID = 0;			// Used for models which are not loaded from a file but from a mesh create from code

		Mesh cubePrimitive;
		Mesh spherePrimitive;
	};
}
