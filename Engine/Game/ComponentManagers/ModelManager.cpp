#include "ModelManager.h"

#include "Game/Game.h"
#include "Game/ComponentManagers/TransformManager.h"
#include "Program/StringID.h"
#include "Program/Log.h"
#include "Program/Allocator.h"
#include "Graphics/VertexArray.h"
#include "Graphics/ResourcesLoader.h"
#include "Graphics/Material.h"
#include "Graphics/MeshDefaults.h"

namespace Engine
{
	void ModelManager::Init(Game *game, unsigned int initialCapacity)
	{
		cubePrimitive = {};
		spherePrimitive = {};
		usedModels = 0;
		disabledModels = 0;
		modelID = 0;

		this->game = game;
		transformManager = &game->GetTransformManager();

		shadowPassID = SID("csm");

		data = {};
		//data.buffer = new unsigned char[initialCapacity * (sizeof(Entity) + sizeof(ModelS) + sizeof(unsigned int) + sizeof(bool) + sizeof(float) + sizeof(AABB) + sizeof(AABB))];
		data.buffer = (unsigned char*)game->GetAllocator()->Allocate(initialCapacity * (sizeof(Entity) + sizeof(ModelS) + sizeof(unsigned int) + sizeof(bool) + sizeof(float) + sizeof(AABB) + sizeof(AABB)));
		data.capacity = initialCapacity;
		data.size = 0;

		data.e = (Entity*)data.buffer;
		data.model = (ModelS*)(data.e + initialCapacity);
		data.pathIndex = (unsigned int*)(data.model + initialCapacity);
		data.castShadows = (bool*)(data.pathIndex + initialCapacity);
		data.lodDistance = (float*)(data.castShadows + initialCapacity);
		data.originalAABB = (AABB*)(data.lodDistance + initialCapacity);
		data.worldSpaceAABB = (AABB*)(data.originalAABB + initialCapacity);

		Log::Print(LogLevel::LEVEL_INFO, "Init Model manager\n");
	}

	void ModelManager::Update()
	{
		// Use transform manager modified transforms on update
		// update aabb if transform changed

		const unsigned int numEnabledModels = usedModels - disabledModels;
		for (unsigned int i = 0; i < numEnabledModels; i++)
		{
			ModelInstance &mi = models[i];
			mi.aabb = utils::RecomputeAABB(mi.model->GetOriginalAABB(), transformManager->GetLocalToWorld(mi.e));

			if (mi.model->GetType() == ModelType::ANIMATED)
			{
				AnimatedModel *am = static_cast<AnimatedModel*>(mi.model);
				am->SetDirty();
				//am->UpdateController();  update when in game
			}
		}
		/*for (unsigned int i = 0; i < usedModels; i++)
		{
			data.worldSpaceAABB[i] = utils::RecomputeAABB(data.originalAABB[i], transformManager->GetLocalToWorld(data.e[i]));
*/
			/*if (mi.model->GetType() == ModelType::ANIMATED)
			{
				AnimatedModel *am = static_cast<AnimatedModel*>(mi.model);
				am->SetDirty();
			}*/
		//}
	}

	void ModelManager::PartialDispose()
	{
		for (unsigned int i = 0; i < usedModels; i++)
		{
			if (models[i].model->GetType() != ModelType::ANIMATED)
				models[i].model->RemoveReference();
		}

		for (auto it = uniqueModels.begin(); it != uniqueModels.end(); it++)
		{
			delete it->second;
		}

		for (size_t i = 0; i < animatedModels.size(); i++)
		{
			delete animatedModels[i];
		}

		for (auto it = animations.begin(); it != animations.end(); it++)
		{
			it->second->RemoveReference();
		}
	}

	void ModelManager::Dispose()
	{
		PartialDispose();

		if (data.buffer)
			game->GetAllocator()->Free(data.buffer);

		Log::Print(LogLevel::LEVEL_INFO, "Disposing Model manager\n");
	}

	void ModelManager::Cull(unsigned int passAndFrustumCount, unsigned int *passIds, const Frustum *frustums, std::vector<VisibilityIndices*> &out)
	{
		const unsigned int numEnabledModels = usedModels - disabledModels;

		/*const glm::vec3 &camPos = game->GetMainCamera()->GetPosition();

		float lodDist = 100000.0f;

		for (unsigned int i = 0; i < passAndFrustumCount; i++)
		{
			for (size_t j = 0; j < numEnabledModels; j++)
			{
				const ModelInstance &mi = models[j];
				lodDist = mi.model->GetLODDistance();

				if (glm::length2(camPos - transformManager->GetLocalPosition(mi.e)) > lodDist)
					continue;

				if (frustums[i].BoxInFrustum(mi.aabb.min, mi.aabb.max) != FrustumIntersect::OUTSIDE)
					out[i]->push_back(j);
			}
		}*/

		/*if (out.size() > 0)
			out[0]->push_back(0);*/

		for (unsigned int i = 0; i < passAndFrustumCount; i++)
		{
			out[i]->push_back(0);
		}
	}

	void ModelManager::GetRenderItems(unsigned int passCount, unsigned int *passIds, const VisibilityIndices &visibility, RenderQueue &outQueues)
	{
		float dt = game->GetDeltaTime();
		const unsigned int numEnabledModels = usedModels - disabledModels;

		if (numEnabledModels > 0)
		{
			for (size_t i = 0; i < numEnabledModels; i++)
			{
				const ModelInstance &mi = models[i];
				Model *model = mi.model;

				const glm::mat4 &localToWorld = transformManager->GetLocalToWorld(mi.e);
				const std::vector<MeshMaterial> &meshesAndMaterials = model->GetMeshesAndMaterials();
				
				if (model->GetType() == ModelType::ANIMATED)
				{
					AnimatedModel *am = static_cast<AnimatedModel*>(model);
					if (am->IsDirty())
						am->UpdateBones(*transformManager, mi.e, dt);

					const std::vector<glm::mat4> &transforms = am->GetBoneTransforms();

					for (size_t j = 0; j < meshesAndMaterials.size(); j++)
					{
						const MeshMaterial &mm = meshesAndMaterials[j];
						const std::vector<ShaderPass> &passes = mm.mat->baseMaterial->GetShaderPasses();

						for (size_t k = 0; k < passCount; k++)
						{
							for (size_t l = 0; l < passes.size(); l++)
							{
								if (passIds[k] == passes[l].queueID)
								{
									RenderItem ri = {};
									ri.mesh = &mm.mesh;
									ri.matInstance = mm.mat;
									ri.shaderPass = l;
									ri.transform = &localToWorld;
									ri.meshParams = &transforms[0][0].x;
									ri.meshParamsSize = transforms.size() * sizeof(glm::mat4);
									outQueues.push_back(ri);
								}
							}
						}
					}
				}
				else
				{
					for (size_t j = 0; j < meshesAndMaterials.size(); j++)
					{
						const MeshMaterial &mm = meshesAndMaterials[j];
						const std::vector<ShaderPass>& passes = mm.mat->baseMaterial->GetShaderPasses();

						for (size_t k = 0; k < passCount; k++)
						{
							for (size_t l = 0; l < passes.size(); l++)
							{
								if (passIds[k] == passes[l].queueID)
								{
									RenderItem ri = {};
									ri.mesh = &mm.mesh;
									ri.matInstance = mm.mat;
									ri.shaderPass = l;
									ri.transform = &localToWorld;
									outQueues.push_back(ri);
								}
							}
						}
					}
				}
			}
		}	

		/*for (auto m : uniqueModels)
		{
			m.second->ClearInstanceData();
		}

		bool hasShadowPass = false;
		for (unsigned int i = 0; i < passCount; i++)
		{
			if (passIds[i] == shadowPassID)
				hasShadowPass = true;
		}

		float dt = game->GetDeltaTime();

		for (size_t i = 0; i < visibility.size(); i++)
		{
			const ModelInstance &mi = models[visibility[i]];
			Model *model = mi.model;

			const std::vector<MeshMaterial> &meshesAndMaterials = model->GetMeshesAndMaterials();
			const glm::mat4 &localToWorld = transformManager->GetLocalToWorld(mi.e);
		
			if (model->GetRefCount() == 1)			// Check if we can use instancing even we only have 1 instance. Like this we avoid this if
			{
				if (hasShadowPass && !model->GetCastShadows())
					continue;

				if (model->GetType() == ModelType::ANIMATED)
				{
					AnimatedModel *am = static_cast<AnimatedModel*>(model);
					if (am->IsDirty())
						am->UpdateBones(*transformManager, mi.e, dt);

					const std::vector<glm::mat4> &transforms = am->GetBoneTransforms();

					for (size_t j = 0; j < meshesAndMaterials.size(); j++)
					{
						const MeshMaterial &mm = meshesAndMaterials[j];
						const std::vector<ShaderPass> &passes = mm.mat->baseMaterial->GetShaderPasses();

						for (size_t k = 0; k < passCount; k++)
						{
							for (size_t l = 0; l < passes.size(); l++)
							{
								if (passIds[k] == passes[l].queueID)
								{
									RenderItem ri = {};
									ri.mesh = &mm.mesh;
									ri.matInstance = mm.mat;
									ri.shaderPass = l;
									ri.transform = &localToWorld;
									ri.meshParams = &transforms[0][0].x;
									ri.meshParamsSize = transforms.size() * sizeof(glm::mat4);
									outQueues.push_back(ri);
								}
							}
						}
					}
				}
				else
				{
					for (size_t j = 0; j < meshesAndMaterials.size(); j++)
					{
						const MeshMaterial &mm = meshesAndMaterials[j];
						const std::vector<ShaderPass> &passes = mm.mat->baseMaterial->GetShaderPasses();

						for (size_t k = 0; k < passCount; k++)
						{
							for (size_t l = 0; l < passes.size(); l++)
							{
								if (passIds[k] == passes[l].queueID)
								{
									RenderItem ri = {};
									ri.mesh = &mm.mesh;
									ri.matInstance = mm.mat;
									ri.shaderPass = l;
									ri.transform = &localToWorld;
									outQueues.push_back(ri);
								}
							}
						}
					}
				}
			}
			else
			{
				model->AddInstanceData(localToWorld);		// Else we use instancing
			}
		}

		for (auto pair : uniqueModels)
		{
			Model *m = pair.second;
			unsigned int instanceDataSize = m->GetInstanceDataSize();

			if (hasShadowPass && !m->GetCastShadows())
				continue;

			if (instanceDataSize > 0)
			{
				const std::vector<MeshMaterial> &meshesAndMaterials = m->GetMeshesAndMaterials();

				for (size_t j = 0; j < meshesAndMaterials.size(); j++)
				{
					const MeshMaterial &mm = meshesAndMaterials[j];
					const std::vector<ShaderPass> &passes = mm.mat->baseMaterial->GetShaderPasses();

					for (size_t k = 0; k < passCount; k++)
					{
						for (size_t l = 0; l < passes.size(); l++)
						{
							if (passIds[k] == passes[l].queueID)
							{
								RenderItem ri = {};
								ri.mesh = &mm.mesh;
								ri.matInstance = mm.mat;
								ri.shaderPass = l;
								ri.instanceData = m->GetInstanceData();

								m->UpdateInstanceInfo(instanceDataSize, 0);

								ri.instanceDataSize = instanceDataSize * sizeof(glm::mat4);
								outQueues.push_back(ri);
							}
						}
					}
				}
			}
		}*/

		/*for (size_t i = 0; i < usedModels; i++)
		{
			//const ModelInstance &mi = models[i];
			//Model *model = mi.model;

			const std::vector<MeshMaterial> &meshes = data.model[i].meshes;

			const glm::mat4 &localToWorld = transformManager->GetLocalToWorld(data.e[i]);

			// Update the model bones only if it is visible
			/*if (model->GetType() == ModelType::ANIMATED)
			{
				AnimatedModel *am = static_cast<AnimatedModel*>(model);
				if (am->IsDirty())
					am->UpdateBones(nullptr, dt);

				const std::vector<glm::mat4> &transforms = am->GetBoneTransforms();

				for (size_t j = 0; j < meshes.size(); j++)
				{
					MaterialInstance *mat = matInstances[j];
					const std::vector<ShaderPass> &passes = mat->baseMaterial->GetShaderPasses();

					for (size_t k = 0; k < passCount; k++)
					{
						for (size_t l = 0; l < passes.size(); l++)
						{
							if (passIds[k] == passes[l].queueID)
							{
								RenderItem ri = {};
								ri.mesh = &meshes[j];
								ri.matInstance = mat;
								//ri.shaderPass = &passes[l];
								ri.shaderPass = l;
								ri.transform = &localToWorld;
								ri.meshParams = &transforms[0][0].x;
								ri.meshParamsSize = transforms.size() * sizeof(glm::mat4);
								outQueues[k].push_back(ri);
							}
						}
					}
				}
			}
			else
			{//
				for (size_t j = 0; j < meshes.size(); j++)
				{
					const MeshMaterial &m = meshes[j];
					const std::vector<ShaderPass> &passes = m.mat->baseMaterial->GetShaderPasses();

					for (size_t k = 0; k < passCount; k++)
					{
						for (size_t l = 0; l < passes.size(); l++)
						{
							if (passIds[k] == passes[l].queueID)
							{
								RenderItem ri = {};
								ri.mesh = &m.mesh;
								ri.matInstance = m.mat;
								ri.shaderPass = l;
								ri.transform = &localToWorld;
								outQueues[k].push_back(ri);
							}
						}
					}
				}
			//}
		}*/
	}

	Model *ModelManager::AddModel(Entity e, const std::string &path, bool animated)
	{
		// Return the model and don't add a new entry if this entity already has a model
		if (map.find(e.id) != map.end())
		{
			return GetModel(e);
		}

		Log::Print(LogLevel::LEVEL_INFO, "Adding model\n");

		Engine::Model *model = LoadModel(path, {}, animated);

		ModelInstance mi = {};
		mi.e = e;
		mi.model = model;
		mi.aabb = { glm::vec3(-0.5f), glm::vec3(0.5f) };
		mi.aabb = utils::RecomputeAABB(model->GetOriginalAABB(), transformManager->GetLocalToWorld(mi.e));

		InsertModelInstance(mi);

		/*LoadModelNew(data.size, path, {}, false, false);

		strcpy(data.path[data.size], path.c_str());
		data.castShadows[data.size] = true;
		data.lodDistance[data.size] = 0.0f;
		data.originalAABB[data.size] = {};
		data.worldSpaceAABB[data.size] = {};

		data.size++;*/

		return model;
	}

	Model *ModelManager::AddModelFromMesh(Entity e, const Mesh &mesh, MaterialInstance *mat, const AABB &aabb)
	{
		// Return the model and don't add a new entry if this entity already has a model
		if (map.find(e.id) != map.end())
		{
			return GetModel(e);
		}

		Engine::Model *model = LoadModel(mesh, mat, aabb);

		ModelInstance mi = {};
		mi.e = e;
		mi.model = model;
		mi.aabb = { glm::vec3(-0.5f), glm::vec3(0.5f) };
		mi.aabb = utils::RecomputeAABB(model->GetOriginalAABB(), transformManager->GetLocalToWorld(mi.e));

		InsertModelInstance(mi);

		return model;
	}

	Model *ModelManager::AddPrimitiveModel(Entity e, ModelType type)
	{
		// Return the model and don't add a new entry if this entity already has a model
		if (map.find(e.id) != map.end())
		{
			return GetModel(e);
		}

		Mesh mesh = LoadPrimitive(type); 

		MaterialInstance *mat = game->GetRenderer()->CreateMaterialInstance(game->GetScriptManager(), "Data/Materials/modelDefault.mat", mesh.vao->GetVertexInputDescs());
		AABB aabb = { glm::vec3(-0.5f, -0.5f, -0.5f), glm::vec3(0.5f, 0.5f, 0.5f) };

		Model *model = new Model(game->GetRenderer(), mesh, mat, aabb, type);

		ModelInstance mi = {};
		mi.e = e;
		mi.model = model;
		mi.aabb = { glm::vec3(-0.5f), glm::vec3(0.5f) };
		mi.aabb = utils::RecomputeAABB(model->GetOriginalAABB(), transformManager->GetLocalToWorld(mi.e));

		InsertModelInstance(mi);

		return model;
	}

	void ModelManager::DuplicateModel(Entity e, Entity newE)
	{
		if (HasModel(e) == false)
			return;

		// Primitive models are created unique, but here we are sharing them, it works but won't work if modifiyng the material instance, which will modify all
		Model *m = GetModel(e);

		Model *newM = m;

		if (m->GetType() == ModelType::ANIMATED)		// Animated models are unique
		{
			const std::vector<MeshMaterial> &meshesAndMaterials = m->GetMeshesAndMaterials();
			size_t size = meshesAndMaterials.size();

			std::vector<std::string> matNames(size);
			
			for (size_t i = 0; i < size; i++)
			{
				matNames[i] = meshesAndMaterials[i].mat->path;
			}

			newM = LoadModel(m->GetPath(), matNames, true);
			newM->SetCastShadows(m->GetCastShadows());
			newM->SetLODDistance(m->GetLODDistance());
			
			AnimatedModel *am = static_cast<AnimatedModel*>(newM);

			const std::vector<Animation*> &modelAnimations = static_cast<AnimatedModel*>(m)->GetAnimations();
			for (size_t i = 0; i < modelAnimations.size(); i++)
			{
				//am->AddAnimation(LoadAnimationFile(animations[i]->path));
				unsigned int index = SID(modelAnimations[i]->path);
				am->AddAnimation(animations[index]);
			}
		}

		ModelInstance mi;
		mi.e = newE;
		mi.model = newM;
		mi.aabb = { glm::vec3(-0.5f), glm::vec3(0.5f) };
		mi.aabb = utils::RecomputeAABB(m->GetOriginalAABB(), transformManager->GetLocalToWorld(mi.e));

		InsertModelInstance(mi);
	}

	void ModelManager::SetModelEnabled(Entity e, bool enable)
	{
		if (HasModel(e))
		{
			if (enable)
			{
				// If we only have one disabled, then there's no need to swap
				if (disabledModels == 1)
				{
					disabledModels--;
					return;
				}
				else
				{
					// To enable when there's more than 1 entity disabled just swap the disabled entity that is going to be enabled with the first disabled entity
					unsigned int entityIndex = map.at(e.id);
					unsigned int firstDisabledEntityIndex = usedModels - disabledModels;		// Don't subtract -1 because we want the first disabled entity, otherwise we would get the last enabled entity. Eg 6 used, 2 disabled, 6-2=4 the first disabled entity is at index 4 and the second at 5

					ModelInstance mi1 = models[entityIndex];
					ModelInstance mi2 = models[firstDisabledEntityIndex];

					models[entityIndex] = mi2;
					models[firstDisabledEntityIndex] = mi1;

					map[e.id] = firstDisabledEntityIndex;
					map[mi2.e.id] = entityIndex;

					disabledModels--;
				}
			}
			else
			{
				// Get the indices of the entity to disable and the last entity
				unsigned int entityIndex = map.at(e.id);
				unsigned int firstDisabledEntityIndex = usedModels - disabledModels - 1;		// Get the first entity disabled or the last entity if none are disabled

				ModelInstance mi1 = models[entityIndex];
				ModelInstance mi2 = models[firstDisabledEntityIndex];

				// Now swap the entities
				models[entityIndex] = mi2;
				models[firstDisabledEntityIndex] = mi1;

				// Swap the indices
				map[e.id] = firstDisabledEntityIndex;
				map[mi2.e.id] = entityIndex;

				disabledModels++;
			}
		}
	}

	void ModelManager::SaveModelPrefab(Serializer &s, Entity e)
	{
		const ModelInstance &mi = models[map.at(e.id)];

		ModelType type = mi.model->GetType();
		s.Write((int)type);

		if (type == ModelType::BASIC)
		{
			s.Write(SID(mi.model->GetPath()));
			mi.model->Serialize(s);
		}
		else if (type == ModelType::ANIMATED)
		{
			AnimatedModel *am = static_cast<AnimatedModel*>(mi.model);
			am->Serialize(s);
		}
		else if (type == ModelType::PRIMITIVE_CUBE || type == ModelType::PRIMITIVE_SPHERE)
		{
			mi.model->Serialize(s);
		}
	}

	void ModelManager::LoadModelPrefab(Serializer &s, Entity e)
	{
		ModelInstance mi = {};
		mi.e = e;

		int t;
		s.Read(t);
		ModelType type = (ModelType)t;

		if (type == ModelType::BASIC)
		{
			unsigned int id;
			s.Read(id);
			// Check if we've already loaded the model. If not then load it and add it to the unique models
			if (uniqueModels.find(id) != uniqueModels.end())
			{
				mi.model = uniqueModels[id];

				// TODO: Fix this
				std::string path;
				bool castShadows = true;
				float lodDistance = 10000.0f;
				AABB originalAABB = {};
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
			}
			else
			{
				mi.model = new Model();
				mi.model->Deserialize(s, game);

				Log::Print(LogLevel::LEVEL_INFO, "Loaded model: %s\n", mi.model->GetPath().c_str());

				uniqueModels[SID(mi.model->GetPath())] = mi.model;
			}
		}
		else if (type == ModelType::ANIMATED)
		{
			AnimatedModel *am = new AnimatedModel();
			am->Deserialize(s, game);

			animatedModels.push_back(am);

			mi.model = am;
		}
		else if (type == ModelType::PRIMITIVE_CUBE || type == ModelType::PRIMITIVE_SPHERE)
		{
			// TODO: This isn't good, we can't new a model here and call deserialize because otherwise it would call LoadModel and we don't have a model to load and also the material names would be gone
					// And we also can't call new Model(mesh, mat,...) because we don't have materials to pass
			std::string path;
			bool castShadows = true;
			float lodDistance = 10000.0f;
			AABB originalAABB = {};

			s.Read(path);
			s.Read(castShadows);
			s.Read(lodDistance);
			s.Read(originalAABB.min);
			s.Read(originalAABB.max);

			unsigned int matCount;
			s.Read(matCount);

			// These types of models can only have one material
			assert(matCount == 1);

			std::string matPath;
			s.Read(matPath);

			Mesh mesh = LoadPrimitive(type);
			MaterialInstance *mat = game->GetRenderer()->CreateMaterialInstance(game->GetScriptManager(), matPath, mesh.vao->GetVertexInputDescs());

			mi.model = new Model(game->GetRenderer(), mesh, mat, { glm::vec3(-0.5f), glm::vec3(0.5f) }, type);
		}

		InsertModelInstance(mi);
	}

	void ModelManager::InsertModelInstance(const ModelInstance &mi)
	{
		if (usedModels < models.size())
		{
			models[usedModels] = mi;
			map[mi.e.id] = usedModels;
		}
		else
		{
			models.push_back(mi);
			map[mi.e.id] = models.size() - 1;
		}

		usedModels++;

		// If there is any disabled entity then we need to swap the new one, which was inserted at the end, with the first disabled entity
		if (disabledModels > 0)
		{
			// Get the indices of the entity to disable and the last entity
			unsigned int newEntityIndex = usedModels - 1;
			unsigned int firstDisabledEntityIndex = usedModels - disabledModels - 1;		// Get the first entity disabled

			ModelInstance mi1 = models[newEntityIndex];
			ModelInstance mi2 = models[firstDisabledEntityIndex];

			// Now swap the entities
			models[newEntityIndex] = mi2;
			models[firstDisabledEntityIndex] = mi1;

			// Swap the indices
			map[mi.e.id] = firstDisabledEntityIndex;
			map[mi2.e.id] = newEntityIndex;
		}
	}

	void ModelManager::RemoveModel(Entity e)
	{
		if (HasModel(e))
		{
			// To remove an entity we need to swap it with the last one, but, because there could be disabled entities at the end
			// we need to first swap the entity to remove with the last ENABLED entity and then if there are any disabled entities, swap the entity to remove again,
			// but this time with the last disabled entity.
			unsigned int entityToRemoveIndex = map.at(e.id);
			unsigned int lastEnabledEntityIndex = usedModels - disabledModels - 1;

			ModelInstance entityToRemoveMi = models[entityToRemoveIndex];
			ModelInstance lastEnabledEntityMi = models[lastEnabledEntityIndex];

			// Swap the entity to remove with the last enabled entity
			models[lastEnabledEntityIndex] = entityToRemoveMi;
			models[entityToRemoveIndex] = lastEnabledEntityMi;

			// Now change the index of the last enabled entity, which is now in the spot of the entity to remove, to the entity to remove index
			map[lastEnabledEntityMi.e.id] = entityToRemoveIndex;
			map.erase(e.id);

			// If there any disabled entities then swap the entity to remove, which is is the spot of the last enabled entity, with the last disabled entity
			if (disabledModels > 0)
			{
				entityToRemoveIndex = lastEnabledEntityIndex;			// The entity to remove is now in the spot of the last enabled entity
				unsigned int lastDisabledEntityIndex = usedModels - 1;

				ModelInstance lastDisabledEntityMi = models[lastDisabledEntityIndex];

				models[lastDisabledEntityIndex] = entityToRemoveMi;
				models[entityToRemoveIndex] = lastDisabledEntityMi;

				map[lastDisabledEntityMi.e.id] = entityToRemoveIndex;
			}

			if (entityToRemoveMi.model->GetType() == ModelType::ANIMATED)
			{
				AnimatedModel *am = static_cast<AnimatedModel*>(entityToRemoveMi.model);
				am->RemoveBoneAttachments(*transformManager);
				
				for (auto it = animatedModels.begin(); it != animatedModels.end(); it++)
				{
					if ((*it) == entityToRemoveMi.model)
					{
						animatedModels.erase(it);
						delete entityToRemoveMi.model;
					}
				}
			}
			else if (entityToRemoveMi.model->GetType() == ModelType::BASIC)
			{
				for (auto it = uniqueModels.begin(); it != uniqueModels.end(); it++)
				{
					if (it->second == entityToRemoveMi.model)
					{
						if (it->second->GetRefCount() == 1)		// Erase when there's only one reference
							uniqueModels.erase(it);

						entityToRemoveMi.model->RemoveReference();
						break;
					}
				}
			}

			usedModels--;
		}
	}

	Model *ModelManager::GetModel(Entity e) const
	{
		return models[map.at(e.id)].model;
	}

	AnimatedModel *ModelManager::GetAnimatedModel(Entity e) const
	{
		if (HasModel(e))
		{
			Model *m = GetModel(e);
			if (m->GetType() == ModelType::ANIMATED)
				return static_cast<AnimatedModel*>(m);
		}

		return nullptr;
	}

	const AABB &ModelManager::GetAABB(Entity e) const
	{
		return models[map.at(e.id)].aabb;
	}

	bool ModelManager::HasModel(Entity e) const
	{
		return map.find(e.id) != map.end();
	}

	bool ModelManager::HasAnimatedModel(Entity e) const
	{
		if (HasModel(e) && GetModel(e)->GetType() == Engine::ModelType::ANIMATED)
			return true;

		return false;
	}

	bool ModelManager::PerformRaycast(Camera *camera, const glm::vec2 &point, Entity &outEntity)
	{
		glm::vec3 dir = utils::GetRayDirection(point, camera);

		//std::cout << dir.x << " " << dir.y << " " << dir.z << "\n";

		float closestDist = 10000.0f;
		int index = -1;
		int objTested = 0;

		const unsigned int numEnabledModels = usedModels - disabledModels;

		for (size_t i = 0; i < numEnabledModels; i++)
		{
			objTested++;

			if (utils::RayAABBIntersection(camera->GetPosition(), dir, models[i].aabb))
			{
				float dist = glm::length2(camera->GetPosition() - glm::vec3(transformManager->GetLocalToWorld(models[i].e)[3]));

				if (dist < closestDist)
				{
					closestDist = dist;
					index = i;
					outEntity = models[i].e;
				}
			}
		}

		//std::cout << "Index: " << index << "\n";
		//std::cout << "Obj tested: " << objTested << "\n";

		if (index >= 0)
			return true;
		else
			return false;
	}

	Model *ModelManager::LoadModel(const std::string &path, const std::vector<std::string> &matNames, bool isAnimated, bool isInstanced, bool loadVertexColors)
	{
		unsigned int id = SID(path);

		// If the model already exists return that one instead
		if (!isAnimated && uniqueModels.find(id) != uniqueModels.end())
		{
			Model *m = uniqueModels[id];
			m->AddReference();
			return m;
		}

		if (isAnimated)
		{
			AnimatedModel *model = new AnimatedModel(game->GetRenderer(), game->GetScriptManager(), path, matNames);

			animatedModels.push_back(model);

			return model;
		}
		else
		{
			Model *model = new Model(game->GetRenderer(), game->GetScriptManager(), path, matNames);

			uniqueModels[id] = model;

			return model;
		}

		Log::Print(LogLevel::LEVEL_INFO, "Done loading model\n");

		return nullptr;
	}

	Animation *ModelManager::LoadAnimation(const std::string &path)
	{
		unsigned int id = SID(path);

		if (animations.find(id) != animations.end())
		{
			animations[id]->AddReference();
			return animations[id];
		}

		Serializer s(game->GetFileManager());
		s.OpenForReading(path);

		if (!s.IsOpen())
		{
			Log::Print(LogLevel::LEVEL_ERROR, "ERROR -> Failed to open animation file: %s\n", path.c_str());
			return nullptr;
		}

		int magic = 0;
		s.Read(magic);

		if (magic != 315)
		{
			Log::Print(LogLevel::LEVEL_ERROR, "ERROR -> Unknown animation file: %s\n", path.c_str());
			return nullptr;
		}

		Animation *a = new Animation;
		a->refCount = 0;
		a->AddReference();
		a->path = path;	
		s.Read(a->name);
		s.Read(a->duration);
		s.Read(a->ticksPerSecond);

		unsigned int size = 0;
		s.Read(size);

		std::string boneName;
		unsigned int numKeyframes = 0;

		for (size_t i = 0; i < size; i++)
		{
			s.Read(boneName);
			s.Read(numKeyframes);
			KeyFrameList keyframes(numKeyframes);
			s.Read(keyframes.data(), numKeyframes * sizeof(Keyframe));

			a->bonesKeyframesList[boneName] = keyframes;
		}
		s.Close();

		animations[id] = a;

		return a;
	}

	Model *ModelManager::LoadModel(const Mesh &mesh, MaterialInstance *mat, const AABB &aabb)
	{
		Model *model = new Model(game->GetRenderer(), mesh, mat, aabb, ModelType::BASIC);
		uniqueModels[modelID] = model;
		modelID++;						// Won't work when serialiazing and then deserializing because we use the path as ID and for this model the path is empty
		return model;
	}

	Mesh ModelManager::LoadPrimitive(ModelType type)
	{
		Mesh mesh = {};

		if (type == ModelType::PRIMITIVE_CUBE)
		{
			// Instead of always loading the mesh, reuse these meshes
			if (!cubePrimitive.vao)
				cubePrimitive = MeshDefaults::CreateCube(game->GetRenderer(), 0.5f);

			mesh = cubePrimitive;
		}
		else if (type == ModelType::PRIMITIVE_SPHERE)
		{
			if (!spherePrimitive.vao)
				spherePrimitive = MeshDefaults::CreateSphere(game->GetRenderer(), 0.5f);

			mesh = spherePrimitive;
		}

		return mesh;
	}

	void ModelManager::AddAnimation(Animation *anim, const std::string &path)
	{
		if (!anim)
			return;

		unsigned int id = SID(path);
		anim->AddReference();
		animations[id] = anim;
	}

	void ModelManager::RemoveModelNoEntity(Model *model)
	{
		for (auto it = uniqueModels.begin(); it != uniqueModels.end(); it++)
		{
			if (it->second == model)
			{
				if (it->second->GetRefCount() == 1)		// Erase when there's only one reference
					uniqueModels.erase(it);

				model->RemoveReference();
				break;
			}
		}
	}

	void ModelManager::Serialize(Serializer &s, bool playMode)
	{
		// Store the map, otherwise we have problems with play/stop when we enable/disable entities
		if (playMode)
		{
			s.Write((unsigned int)map.size());
			for (auto it = map.begin(); it != map.end(); it++)
			{
				s.Write(it->first);
				s.Write(it->second);
			}
		}

		s.Write(static_cast<unsigned int>(uniqueModels.size()));
		for (auto it = uniqueModels.begin(); it != uniqueModels.end(); it++)
		{
			it->second->Serialize(s);
		}

		s.Write(static_cast<unsigned int>(animatedModels.size()));
		for (size_t i = 0; i < animatedModels.size(); i++)
		{
			AnimatedModel *am = static_cast<AnimatedModel*>(animatedModels[i]);
			am->Serialize(s);
		}

		s.Write(usedModels);
		s.Write(disabledModels);
		for (unsigned int i = 0; i < usedModels; i++)
		{
			const ModelInstance &mi = models[i];
			s.Write(mi.e.id);

			ModelType type = mi.model->GetType();
			s.Write((int)type);

			if (type == ModelType::BASIC)
			{
				s.Write(SID(mi.model->GetPath()));
			}
			else if (type == ModelType::ANIMATED)
			{
				for (size_t j = 0; j < animatedModels.size(); j++)			// Stored animated models separately so we don't have to search for the index
				{
					if (animatedModels[j] == mi.model)
					{
						s.Write(static_cast<unsigned int>(j));
						break;
					}
				}
			}
			else if (type == ModelType::PRIMITIVE_CUBE || type == ModelType::PRIMITIVE_SPHERE)
			{
				mi.model->Serialize(s);
			}
		}
	}

	void ModelManager::Deserialize(Serializer &s, bool playMode)
	{
		Log::Print(LogLevel::LEVEL_INFO, "Deserializing model manager\n");

		if (!playMode)
		{
			unsigned int uniqueModelsCount = 0;
			s.Read(uniqueModelsCount);

			Log::Print(LogLevel::LEVEL_INFO, "Num unique models: %u\n", uniqueModelsCount);

			for (unsigned int i = 0; i < uniqueModelsCount; i++)
			{
				Model *m = new Model();
				m->Deserialize(s, game);

				Log::Print(LogLevel::LEVEL_INFO, "Loaded model: %s\n", m->GetPath().c_str());

				uniqueModels[SID(m->GetPath())] = m;
			}		

			unsigned int animatedModelsCount = 0;
			s.Read(animatedModelsCount);
			animatedModels.resize(animatedModelsCount);

			Log::Print(LogLevel::LEVEL_INFO, "Num animated models: %u\n", animatedModelsCount);

			for (unsigned int i = 0; i < animatedModelsCount; i++)
			{
				AnimatedModel *am = new AnimatedModel();
				am->Deserialize(s, game);

				Log::Print(LogLevel::LEVEL_INFO, "Loaded animated model: %s\n", am->GetPath().c_str());

				animatedModels[i] = am;
			}

			s.Read(usedModels);
			s.Read(disabledModels);
			models.resize(usedModels);

			for (unsigned int i = 0; i < usedModels; i++)
			{
				ModelInstance mi;
				s.Read(mi.e.id);

				int type = 0;
				s.Read(type);

				ModelType modelType = (ModelType)type;

				if (modelType == ModelType::BASIC)
				{
					unsigned int id;
					s.Read(id);
					mi.model = uniqueModels[id];
				}
				else if(modelType == ModelType::ANIMATED)
				{
					unsigned int index;
					s.Read(index);
					mi.model = animatedModels[index];
				}
				else if (modelType == ModelType::PRIMITIVE_CUBE || modelType == ModelType::PRIMITIVE_SPHERE)
				{
					// TODO: This isn't good, we can't new a model here and call deserialize because otherwise it would call LoadModel and we don't have a model to load and also the material names would be gone
					// And we also can't call new Model(mesh, mat,...) because we don't have materials to pass
					std::string path;
					bool castShadows = true;
					float lodDistance = 10000.0f;
					AABB originalAABB = {};

					s.Read(path);
					s.Read(castShadows);
					s.Read(lodDistance);
					s.Read(originalAABB.min);
					s.Read(originalAABB.max);

					unsigned int matCount;
					s.Read(matCount);

					// These types of models can only have one material
					assert(matCount == 1);

					std::string matPath;
					s.Read(matPath);

					Mesh mesh = LoadPrimitive(modelType);
					MaterialInstance *mat = game->GetRenderer()->CreateMaterialInstance(game->GetScriptManager(), matPath, mesh.vao->GetVertexInputDescs());

					mi.model = new Model(game->GetRenderer(), mesh, mat, {glm::vec3(-0.5f), glm::vec3(0.5f)}, modelType);
					//mi.model->Deserialize(s, game, true);			// Use reload true so we don't load the model
				}

				models[i] = mi;
				map[mi.e.id] = i;
			}
		}
		else
		{
			// Read the map to prevent bugs when entities are enabled/disabled with play/stop
			unsigned int mapSize = 0;
			s.Read(mapSize);

			unsigned int eid, idx;
			for (unsigned int i = 0; i < mapSize; i++)
			{
				s.Read(eid);
				s.Read(idx);
				map[eid] = idx;
			}


			unsigned int uniqueModelsCount = 0;
			s.Read(uniqueModelsCount);
			for (auto it = uniqueModels.begin(); it != uniqueModels.end(); it++)
			{
				it->second->Deserialize(s, game, true);
			}

			unsigned int animatedModelsCount = 0;
			s.Read(animatedModelsCount);

			for (size_t i = 0; i < animatedModels.size(); i++)
			{
				animatedModels[i]->Deserialize(s, game, true);
			}

			s.Read(usedModels);
			s.Read(disabledModels);

			for (unsigned int i = 0; i < usedModels; i++)
			{		
				s.Read(eid);
				unsigned int idx = map[eid];

				ModelInstance &mi = models[idx];			// Check if idx is needed
				mi.e.id = eid;
				//s.Read(mi.e.id);

				int type = 0;
				s.Read(type);

				ModelType modelType = (ModelType)type;

				if (modelType == ModelType::BASIC)
				{
					unsigned int id;
					s.Read(id);
					mi.model = uniqueModels[id];
				}
				else if(modelType == ModelType::ANIMATED)
				{
					unsigned int index;
					s.Read(index);
					mi.model = animatedModels[index];
				}
				else if (modelType == ModelType::PRIMITIVE_CUBE || modelType == ModelType::PRIMITIVE_SPHERE)
				{
					// While this isn't fixed just read the data
					std::string path;
					bool castShadows = true;
					float lodDistance = 10000.0f;
					AABB originalAABB = {};

					s.Read(path);
					s.Read(castShadows);
					s.Read(lodDistance);
					s.Read(originalAABB.min);
					s.Read(originalAABB.max);

					unsigned int matCount;
					s.Read(matCount);

					// These types of models can only have one material
					assert(matCount == 1);

					std::string matPath;
					s.Read(matPath);
				}
			}
		}
	}

	/*void ModelManager::LoadModelNew(unsigned int index, const std::string &path, const std::vector<std::string> &matNames, bool isInstanced, bool loadVertexColors)
	{
		data.originalAABB[index].min = glm::vec3(100000.0f);
		data.originalAABB[index].max = glm::vec3(-100000.0f);

		Assimp::Importer importer;
		const aiScene* scene = importer.ReadFile(path, aiProcess_JoinIdenticalVertices | aiProcess_FlipUVs); //| aiProcess_GenSmoothNormals); //| aiProcess_CalcTangentSpace);

		if (!scene || scene->mFlags == AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
		{
			Log::Print(LogLevel::LEVEL_ERROR, "Assimp Error: %s\n", importer.GetErrorString());
			return;
		}

		// Load all the model meshes
		for (unsigned int i = 0; i < scene->mNumMeshes; i++)
		{
			const aiMesh* aiMesh = scene->mMeshes[i];

			if (matNames.size() > 0)
			{
				Mesh mesh = ProcessMesh(index, aiMesh, scene, isInstanced, loadVertexColors);
				MaterialInstance *mat = game->GetRenderer()->CreateMaterialInstance(game->GetScriptManager(), matNames[data.model[i].meshes.size()], mesh.vao->GetVertexInputDescs());
				MeshMaterial m = { mesh,mat };
				data.model[index].meshes.push_back(m);
			}
			else
			{
				// Workaround for terrain vegetation to work when adding through the editor with no materials
				if (isInstanced == false)
				{
					Mesh mesh = ProcessMesh(index, aiMesh, scene, isInstanced, loadVertexColors);		// If we're loading the model without providing material names then we're loading it																															
					MaterialInstance *mat = game->GetRenderer()->CreateMaterialInstance(game->GetScriptManager(), "Data/Materials/modelDefault.mat", mesh.vao->GetVertexInputDescs());
					MeshMaterial m = { mesh,mat };
					data.model[index].meshes.push_back(m);
				}
				else                                                                                                                // through the editor so load our default material
				{
					Mesh mesh = ProcessMesh(index, aiMesh, scene, isInstanced, loadVertexColors);
					MaterialInstance *mat = game->GetRenderer()->CreateMaterialInstance(game->GetScriptManager(), "Data/Materials/modelDefaultInstanced.mat", mesh.vao->GetVertexInputDescs());
					MeshMaterial m = { mesh,mat };
					data.model[index].meshes.push_back(m);
				}
			}
		}
	}

	Mesh ModelManager::ProcessMesh(unsigned int index, const aiMesh *aimesh, const aiScene *aiscene, bool isInstanced, bool loadVertexColors)
	{
		std::vector<unsigned short> indices;

		for (unsigned int i = 0; i < aimesh->mNumFaces; i++)
		{
			aiFace face = aimesh->mFaces[i];

			for (unsigned int j = 0; j < face.mNumIndices; j++)
				indices.push_back(face.mIndices[j]);
		}

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

				data.originalAABB[index].min = glm::min(data.originalAABB[index].min, v.pos);
				data.originalAABB[index].max = glm::max(data.originalAABB[index].max, v.pos);

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

			Buffer *vb = game->GetRenderer()->CreateVertexBuffer(vertices.data(), vertices.size() * sizeof(VertexPOS3D_UV_NORMAL_COLOR), BufferUsage::STATIC);
			Buffer *ib = game->GetRenderer()->CreateIndexBuffer(indices.data(), indices.size() * sizeof(unsigned short), BufferUsage::STATIC);

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

				m.vao = game->GetRenderer()->CreateVertexArray(descs, 2, { vb }, ib);
			}
			else
			{
				m.vao = game->GetRenderer()->CreateVertexArray(&desc, 1, { vb }, ib);
			}

			// Prevent the min and max from being both 0 (when loading a plane for example)
			if (data.originalAABB[index].min.y < 0.001f && data.originalAABB[index].min.y > -0.001f)
				data.originalAABB[index].min.y = -0.01f;
			if (data.originalAABB[index].max.y < 0.001f && data.originalAABB[index].max.y > -0.001f)
				data.originalAABB[index].max.y = 0.01f;

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

				data.originalAABB[index].min = glm::min(data.originalAABB[index].min, v.pos);
				data.originalAABB[index].max = glm::max(data.originalAABB[index].max, v.pos);

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

			Buffer *vb = game->GetRenderer()->CreateVertexBuffer(vertices.data(), vertices.size() * sizeof(VertexPOS3D_UV_NORMAL), BufferUsage::STATIC);
			Buffer *ib = game->GetRenderer()->CreateIndexBuffer(indices.data(), indices.size() * sizeof(unsigned short), BufferUsage::STATIC);

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

				m.vao = game->GetRenderer()->CreateVertexArray(descs, 2, { vb }, ib);
			}
			else
			{
				m.vao = game->GetRenderer()->CreateVertexArray(&desc, 1, { vb }, ib);
			}

			// Prevent the min and max from being both 0 (when loading a plane for example)
			if (data.originalAABB[index].min.y < 0.001f && data.originalAABB[index].min.y > -0.001f)
				data.originalAABB[index].min.y = -0.01f;
			if (data.originalAABB[index].max.y < 0.001f && data.originalAABB[index].max.y > -0.001f)
				data.originalAABB[index].max.y = 0.01f;

			return m;
		}
	}*/
}
