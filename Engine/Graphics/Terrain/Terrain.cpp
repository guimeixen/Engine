#include "Terrain.h"

#include "Program/Log.h"
#include "Graphics/Material.h"
#include "Graphics/ResourcesLoader.h"
#include "Graphics/Renderer.h"
#include "Graphics/Mesh.h"
#include "Graphics/Buffers.h"
#include "Graphics/VertexArray.h"
#include "Program/Input.h"
#include "Game/Game.h"
#include "Graphics/Model.h"
#include "Program/Random.h"
#include "Program/Utils.h"
#include "Program/StringID.h"

#include "Physics/Collider.h"

#include "include/glm/gtc/matrix_transform.hpp"
#include "include/glm/gtx/quaternion.hpp"
#include "include/stb_image.h"

#include <iostream>
#include <fstream>
#include <random>

namespace Engine
{
	Terrain::Terrain()
	{
		heights = nullptr;
		matInstance = nullptr;
		game = nullptr;
		mesh.vao = nullptr;
		terrainInstancingBuffer = nullptr;
		vegInstancingBufferLOD0 = nullptr;
		vegInstancingBufferLOD1 = nullptr;
		vegInstancingBufferLOD2 = nullptr;
	}

	Terrain::~Terrain()
	{
	}

	bool Terrain::Init(Game *game, const TerrainInfo &terrainInfo)
	{
		this->game = game;
		renderer = game->GetRenderer();

		VertexAttribute posUv = {};
		posUv.count = 4;
		posUv.offset = 0;
		posUv.vertexAttribFormat = VertexAttributeFormat::FLOAT;

		VertexAttribute instance = {};
		instance.count = 4;
		instance.offset = 0;
		instance.vertexAttribFormat = VertexAttributeFormat::FLOAT;

		terrainInputDescs.resize(2);
		terrainInputDescs[0].stride = 4 * sizeof(float);
		terrainInputDescs[0].attribs = { posUv };
		terrainInputDescs[0].instanced = false;

		terrainInputDescs[1].stride = 4 * sizeof(float);
		terrainInputDescs[1].attribs = { instance };
		terrainInputDescs[1].instanced = true;

		matInstance = renderer->CreateMaterialInstance(game->GetScriptManager(), terrainInfo.matPath, terrainInputDescs);
		matInstance->baseMaterial->SetShowInEditor(false);

		if (matInstance->textures.size() < 1)
			return false;

		SetHeightmap(matInstance->textures[0]->GetPath());

		// Create vegetation grid
		/*unsigned int count = resolution / vegCellSize;
		vegCells.resize(count * count);

		for (unsigned int z = 0; z < count; z++)
		{
			for (unsigned int x = 0; x < count; x++)
			{
				VegetationCell &cell = vegCells[z * count + x];
				cell.x = x * vegCellSize;
				cell.z = z * vegCellSize;
				cell.aabb = { glm::vec3(cell.x, 0.0f, cell.z), glm::vec3(cell.x + vegCellSize, 10.0f, cell.z + vegCellSize) };
			}
		}*/

		LoadVegetationFile(terrainInfo.vegPath);

		// If vegInstanceBuffer is still null it means that we didn't load any vegetation so create it to be ready for adding vegetation
		if (!vegInstancingBufferLOD0)
			CreateVegInstanceBuffer();

		CreateVegColliders();

		opaquePassID = SID("opaque");
		csmPassID = SID("csm");

		return true;
	}

	void Terrain::Cull(unsigned int passAndFrustumCount, unsigned int *passIds, const Frustum *frustums, std::vector<VisibilityIndices*> &out)
	{
		culledVegInstDataLOD0.clear();
		culledVegInstDataLOD1.clear();
		culledVegInstDataLOD2.clear();

		for (unsigned int i = 0; i < passAndFrustumCount; i++)
		{
			if (passIds[i] != opaquePassID && passIds[i] != csmPassID)
				continue;

			const glm::vec3 &camPos = game->GetMainCamera()->GetPosition();		// Don't use the same camera for every frustum, eg csm should use another camera

			for (size_t j = 0; j < vegetation.size(); j++)
			{
				Vegetation &v = vegetation[j];
				v.renderLOD0 = false;
				v.renderLOD1 = false;
				v.renderLOD2 = false;

				const AABB &aabb = v.model->GetOriginalAABB();			// We use the lod0 aabb and don't switch aabb between lods because they are very similar
				float radius = glm::max(aabb.max.x, glm::max(aabb.max.y, aabb.max.z));

				const size_t offsetLOD0 = culledVegInstDataLOD0.size();
				const size_t offsetLOD1 = culledVegInstDataLOD1.size();
				const size_t offsetLOD2 = culledVegInstDataLOD2.size();

				for (size_t k = v.offset; k < v.offset + v.count; k++)
				{
					const ModelInstanceData &m = vegetationInstData[k];
					float distSqr = glm::length2(camPos - glm::vec3(m.modelMatrix[3]));

					if (distSqr > v.lod2Dist)
					{
						if (frustums[i].SphereInFrustum(m.modelMatrix[3], radius) != FrustumIntersect::OUTSIDE)
							culledVegInstDataLOD2.push_back(m);
					}
					else if (distSqr > v.lod1Dist)
					{
						if (frustums[i].SphereInFrustum(m.modelMatrix[3], radius) != FrustumIntersect::OUTSIDE)
							culledVegInstDataLOD1.push_back(m);
					}
					else
					{
						if (frustums[i].SphereInFrustum(m.modelMatrix[3], radius) != FrustumIntersect::OUTSIDE)
							culledVegInstDataLOD0.push_back(m);
					}
				}

				bool idSet = false;
				if (offsetLOD0 != culledVegInstDataLOD0.size())		// If we've added a model of this veg type to the culled list
				{
					v.model->UpdateInstanceInfo(culledVegInstDataLOD0.size() - offsetLOD0, offsetLOD0);
					out[i]->push_back(j);
					idSet = true;
					v.renderLOD0 = true;
				}
				if (offsetLOD1 != culledVegInstDataLOD1.size())
				{
					if (v.modelLOD1)
					{
						v.modelLOD1->UpdateInstanceInfo(culledVegInstDataLOD1.size() - offsetLOD1, offsetLOD1);
						if (!idSet)
						{
							out[i]->push_back(j);
							idSet = true;
						}
						v.renderLOD1 = true;
					}
				}
				if (offsetLOD2 != culledVegInstDataLOD2.size())
				{
					if (v.modelLOD2)
					{
						v.modelLOD2->UpdateInstanceInfo(culledVegInstDataLOD2.size() - offsetLOD2, offsetLOD2);
						if (!idSet)
						{
							out[i]->push_back(j);
							//idSet = true;
						}
						v.renderLOD2 = true;
					}
				}
			}

			// Put terrain visibility index at last
			if (data.size() > 0)
				out[i]->push_back(0);
		}

		if (culledVegInstDataLOD0.size() > 0)
			vegInstancingBufferLOD0->Update(culledVegInstDataLOD0.data(), culledVegInstDataLOD0.size() * sizeof(ModelInstanceData), 0);
		if (culledVegInstDataLOD1.size() > 0)
			vegInstancingBufferLOD1->Update(culledVegInstDataLOD1.data(), culledVegInstDataLOD1.size() * sizeof(ModelInstanceData), 0);
		if (culledVegInstDataLOD2.size() > 0)
			vegInstancingBufferLOD2->Update(culledVegInstDataLOD2.data(), culledVegInstDataLOD2.size() * sizeof(ModelInstanceData), 0);

		//unsigned int culled = vegetationInstData.size() - (culledVegInstDataLOD0.size() + culledVegInstDataLOD1.size() + culledVegInstDataLOD2.size());
			//std::cout << culled << '\n';
	}

	void Terrain::GetRenderItems(unsigned int passCount, unsigned int *passIds, const VisibilityIndices &visibility, RenderQueue &outQueues)
	{
		struct Match
		{
			unsigned int outQueueIndex;
			unsigned int passIndex;
			MaterialInstance *matInstance;
			unsigned short matIndex;
		};

		static const unsigned short MAX_PASSES = 8;

		for (size_t i = 0; i < visibility.size() - 1; i++)		// Don't check the last index because it's the terrain visibility index
		{
			Vegetation &v = vegetation[visibility[i]];

			unsigned short numMatching = 0;
			Match matches[MAX_PASSES] = {};

			// Because the lod models of a vegetation use the same material we can
			// just find the matching passes one time instead of for each lod
			const std::vector<MeshMaterial> &meshesAndMaterials = v.model->GetMeshesAndMaterials();
			for (size_t j = 0; j < meshesAndMaterials.size(); j++)
			{
				MaterialInstance *matInst = meshesAndMaterials[j].mat;

				const std::vector<ShaderPass> &passes = matInst->baseMaterial->GetShaderPasses();
				for (unsigned int k = 0; k < passCount; k++)
				{
					for (size_t l = 0; l < passes.size(); l++)
					{
						if (passIds[k] == passes[l].queueID)
							matches[numMatching++] = { k, static_cast<unsigned int>(l), matInst, static_cast<unsigned short>(j) };
					}
				}
			}

			// LOD0
			if (v.renderLOD0)
			{
				const std::vector<MeshMaterial> &meshesAndMaterials = v.model->GetMeshesAndMaterials();

				for (unsigned short i = 0; i < numMatching; i++)
				{
					const Match &m = matches[i];

					RenderItem ri = {};
					ri.mesh = &meshesAndMaterials[m.matIndex].mesh;		// We can use the material index because there are for every mesh there is only one material
					ri.matInstance = m.matInstance;
					ri.shaderPass = m.passIndex;
					//outQueues[m.outQueueIndex].push_back(ri);
					outQueues.push_back(ri);
				}
			}

			// LOD1
			if (v.renderLOD1)
			{
				const std::vector<MeshMaterial> &meshesAndMaterials = v.modelLOD1->GetMeshesAndMaterials();

				for (unsigned short i = 0; i < numMatching; i++)
				{
					const Match &m = matches[i];

					RenderItem ri = {};
					ri.mesh = &meshesAndMaterials[m.matIndex].mesh;		// We can use the material index because there are for every mesh there is only one material
					ri.matInstance = m.matInstance;
					ri.shaderPass = m.passIndex;
					//outQueues[m.outQueueIndex].push_back(ri);
					//outQueues[m.outQueueIndex].push_back(ri);
					outQueues.push_back(ri);
				}
			}

			// LOD2
			if (v.renderLOD2)
			{
				const std::vector<MeshMaterial> &meshesAndMaterials = v.modelLOD2->GetMeshesAndMaterials();

				for (unsigned short i = 0; i < numMatching; i++)
				{
					const Match &m = matches[i];

					RenderItem ri = {};
					ri.mesh = &meshesAndMaterials[m.matIndex].mesh;		// We can use the material index because there are for every mesh there is only one material
					ri.matInstance = m.matInstance;
					ri.shaderPass = m.passIndex;
					//outQueues[m.outQueueIndex].push_back(ri);
					outQueues.push_back(ri);
				}
			}
		}

		//renderer->GetFont().AddText("Veg instances: " + std::to_string(culledVegInstData.size()), glm::vec2(10.0f, (float)(renderer->GetHeight() - renderer->GetHeight() * 0.14f)), glm::vec2(0.25f));
		//renderer->GetFont().AddText("Culled veg instances: " + std::to_string(culled), glm::vec2(10.0f, (float)(renderer->GetHeight() - renderer->GetHeight() * 0.16f)), glm::vec2(0.25f));

		if (data.size() <= 0)
			return;

		bool anyMatches = false;
		const std::vector<ShaderPass> &passes = matInstance->baseMaterial->GetShaderPasses();
		for (size_t i = 0; i < passCount; i++)
		{
			for (size_t j = 0; j < passes.size(); j++)
			{
				if (passIds[i] == passes[j].queueID)
				{
					RenderItem ri = {};
					ri.mesh = &mesh;
					ri.matInstance = matInstance;
					ri.shaderPass = j;

					//outQueues[i].push_back(ri);
					outQueues.push_back(ri);
					anyMatches = true;
				}
			}
		}

		if (!anyMatches)
			return;

		if (!isTerrainDataUpdated)
		{
			terrainInstancingBuffer->Update(data.data(), data.size() * sizeof(TerrainInstanceData), 0);
			isTerrainDataUpdated = true;
		}

		mesh.vertexOffset = 0;
		mesh.instanceCount = data.size();

		/*if (updateMaterialUBO)
		{
			UniformBuffer *matUbo = matInstance->baseMaterial->GetMaterialUBO();

			TerrainMaterialUBO uboData;
			uboData.terrainParams = glm::vec2((float)resolution, heightScale);
			uboData.selectionPointAndRadius = glm::vec3(intersectionPoint.x / resolution, intersectionPoint.z / resolution, vegBrushRadius / resolution);

			matUbo->Update(&uboData, sizeof(TerrainMaterialUBO), 0);

			updateMaterialUBO = false;
		}*/

		
	}

	void Terrain::UpdateLOD(Camera *camera)
	{
		/*if (Input::IsKeyPressed(KEY_U))
		{
			lodFar += 0.5f;
			CalculateLODVisRanges();
			std::cout << "Lod far: " << lodFar << "\n";
		}
		else if (Input::IsKeyPressed(KEY_T))
		{
			lodFar -= 0.5f;
			CalculateLODVisRanges();
			std::cout << "Lod far: " << lodFar << "\n";
		}*/

		data.clear();
		for (size_t z = 0; z < nodes.size(); z++)
		{
			for (size_t x = 0; x < nodes[0].size(); x++)
			{
				nodes[z][x]->LODSelect(lodVisRanges, lodLevels - 1, camera, data);
			}
		}
		isTerrainDataUpdated = false;
	}

	void Terrain::UpdateVegColliders(Camera *camera)
	{
		if (vegetation.size() == 0 || !vegColCreated)
			return;

		//const glm::vec3 &camPos = camera->GetPosition();

		for (size_t i = 0; i < vegetation.size(); i++)
		{
			Vegetation &v = vegetation[i];
			if (!v.generateColliders || v.count == 0)
				continue;

			for (size_t j = v.offset; j < v.offset + v.count; j++)
			{
				const glm::vec3 &pos = vegetationInstData[j].modelMatrix[3];
				//float dist2 = glm::length2(camPos - pos);
				float dist2 = glm::length2(collidersRefPoint - pos);

				for (size_t k = 0; k < maxColliders; k++)
				{
					VegColInfo &vci = closestColliders[k];
					if (dist2 < vci.dist2)
					{
						vci.dist2 = dist2;
						vci.position = pos;
						break;
					}
				}
			}
		}

		for (size_t i = 0; i < maxColliders; i++)
		{
			VegColInfo &vci = closestColliders[i];
			const glm::vec3 &pos = vci.position;
			vci.col->SetPosition(glm::vec3(pos.x, pos.y, pos.z));
			vci.dist2 = 100000.0f;
		}
	}

	void Terrain::Dispose()
	{
		if (heights)
		{
			delete[] heights;
			heights = nullptr;
		}

		if (mesh.vao)
		{
			delete mesh.vao;
			mesh.vao = nullptr;
		}

		for (size_t j = 0; j < matInstance->textures.size(); j++)
		{
			if (matInstance->textures[j])
				matInstance->textures[j]->RemoveReference();
		}

		/*for (size_t i = 0; i < vegetation.size(); i++)
		{
			game->GetModelManager().RemoveModelNoEntity(vegetation[i].model);
		}*/

		if (obstacleMap)
			stbi_image_free(obstacleMap);

		for (size_t z = 0; z < nodes.size(); z++)
		{
			for (size_t x = 0; x < nodes[z].size(); x++)
			{
				delete nodes[z][x];
			}
		}
	}

	RenderItem Terrain::GetTerrainRenderItem()
	{
		RenderItem ri = {};
		ri.mesh = &mesh;
		ri.matInstance = matInstance;
		ri.shaderPass = 0;

		if (!isTerrainDataUpdated)
		{
			terrainInstancingBuffer->Update(data.data(), data.size() * sizeof(TerrainInstanceData), 0);
			isTerrainDataUpdated = true;
		}

		return ri;
	}

	void Terrain::UpdateEditing()
	{
		isBeingEdited = false;

		if (Input::IsMouseButtonDown(0))
		{
			float delta = 0.25f;
			float mint = 0.1f;		// cam near plane
			float maxt = 200.0f;

			glm::vec3 rayOrigin = game->GetMainCamera()->GetPosition();
			glm::vec3 rayDir = utils::GetRayDirection(Input::GetMousePosition(), game->GetMainCamera());

			for (float t = mint; t < maxt; t += delta)
			{
				intersectionPoint = rayOrigin + rayDir * t;

				float height = GetExactHeightAt(intersectionPoint.x, intersectionPoint.z);

				if (intersectionPoint.y < height)												// If we are below the terrain...
				{
					//interPoint.y = height;
					intersectionPoint = ((rayOrigin + rayDir * t) + (rayOrigin + rayDir * (t - delta))) / 2.0f;
					//rayIntersected = true;

					if (InBounds((int)intersectionPoint.x, (int)intersectionPoint.z))
					{
						//if (paintingTerrain)
							//Paint(deltaTime);
						//PaintLowPoly(deltaTime);
						//else
							DeformTerrain();

							isBeingEdited = true;
					}

					break;
				}
				//delta = 0.5f * t;
			}
		}
//		rayIntersected = false;
	}

	void Terrain::EnableEditing()
	{
		editingEnabled = true;
	}

	void Terrain::DisableEditing()
	{
		editingEnabled = false;
	}

	void Terrain::DeformTerrain()
	{
		int x = (int)intersectionPoint.z;
		int z = (int)intersectionPoint.x;

		if (!InBounds(x, z))
			return;

		//float h = heights[x * resolution + z];
		float deltaTime = game->GetDeltaTime();

		glm::vec2 c = glm::vec2((float)x, (float)z);

		if (deformType == DeformType::RAISE || deformType == DeformType::FLATTEN)
		{
			for (size_t xx = x - brushRadius; xx < x + brushRadius; xx++)
			{
				for (size_t zz = z - brushRadius; zz < z + brushRadius; zz++)
				{
					glm::vec2 d = glm::vec2((float)xx, (float)zz) - c;

					if ((d.x*d.x + d.y*d.y < brushRadius * brushRadius) && InBounds(xx, zz))
					{
						float dist = glm::length(glm::vec2(xx, zz) - c) * 3.14159f / brushRadius;

						heights[xx * resolution + zz] += (0.5f + 0.5f * glm::cos(dist)) * brushStrength * deltaTime;

						if (deformType == DeformType::FLATTEN && heights[xx * resolution + zz] > flattenHeight)
						{
							heights[xx * resolution + zz] = flattenHeight;
						}

						if (heights[xx * resolution + zz] > 255.0f)
							heights[xx * resolution + zz] = 255.0f;
					}
				}
			}
		}
	}

	void Terrain::AddVegetation(const std::string &modelPath)
	{
		for (size_t i = 0; i < vegetation.size(); i++)
		{
			if (vegetation[i].model->GetPath() == modelPath)			// Don't add duplicates
				return;
		}

		Vegetation v = {};
		v.heightOffset = 0.0f;
		v.capacity = 0;
		v.count = 0;
		v.density = 1;
		v.lodDist = 2000.0f;
		/*v.lod1Dist = 50.0f;
		v.lod2Dist = 100.0f;*/
		v.lod1Dist = 1500.0f;
		v.lod2Dist = 3000.0f;
		v.maxSlope = 0.0f;
		//v.model = game->GetModelManager().LoadModel(modelPath, false, true, false);		// Add model from custom format
		v.modelLOD1 = nullptr;
		v.modelLOD2 = nullptr;
		v.minScale = 1.0f;
		v.maxScale = 1.0f;
		v.spacing = 1;
		v.generateObstacles = false;
		v.generateColliders = false;

		if (vegetation.size() > 0)
			v.offset = vegetation[vegetation.size() - 1].offset + vegetation[vegetation.size() - 1].capacity;
		else
			v.offset = 0;

		// Add the vegetation instancing buffer to the model's mesh's vaos
		const std::vector<MeshMaterial> &meshesAndMaterials = v.model->GetMeshesAndMaterials();
		for (size_t i = 0; i < meshesAndMaterials.size(); i++)
		{
			AddVegInstanceBufferToMesh(meshesAndMaterials[i].mesh, 0);
		}

		vegetation.push_back(v);
	}

	void Terrain::ChangeVegetationModel(Vegetation &v, int lod, const std::string &newModelPath)
	{
		if (lod == 1)
		{
			// Crashes because we delete the veginstancingbuffer FIX THIS
			/*if (v.modelLOD1)
				Engine::ResourcesLoader::RemoveModel(v.modelLOD1);*/

			//v.modelLOD1 = game->GetModelManager().LoadModel(newModelPath, false, true, false);		// Add model from custom format
			

			if (v.modelLOD1)
			{
				// Add the vegetation instancing buffer to the model's mesh's vaos
				const std::vector<MeshMaterial> &meshesAndMaterials = v.modelLOD1->GetMeshesAndMaterials();
				for (size_t i = 0; i < meshesAndMaterials.size(); i++)
				{
					AddVegInstanceBufferToMesh(meshesAndMaterials[i].mesh, 1);
					v.modelLOD1->SetMeshMaterial((unsigned short)i, v.model->GetMaterialInstanceOfMesh((unsigned short)i));
				}
			}
		}
		else if (lod == 2)
		{
			// Crashes because we delete the veginstancingbuffer FIX THIS
			/*if (v.modelLOD2)
				Engine::ResourcesLoader::RemoveModel(v.modelLOD2);*/

			//v.modelLOD2 = game->GetModelManager().LoadModel(newModelPath, false, true, false);					// Add model from custom format

			if (v.modelLOD2)
			{
				// Add the vegetation instancing buffer to the model's mesh's vaos
				const std::vector<MeshMaterial> &meshesAndMaterials = v.modelLOD2->GetMeshesAndMaterials();
				for (size_t i = 0; i < meshesAndMaterials.size(); i++)
				{
					AddVegInstanceBufferToMesh(meshesAndMaterials[i].mesh, 2);
					v.modelLOD2->SetMeshMaterial((unsigned short)i, v.model->GetMaterialInstanceOfMesh((unsigned short)i));
				}
			}
		}
	}

	void Terrain::PaintVegetation(const std::vector<int> &ids, const glm::vec3 &rayOrigin, const glm::vec3 &rayDir)
	{
		if (vegetation.size() <= 0)
			return;

		IntersectTerrain(rayOrigin, rayDir, intersectionPoint);

		for (size_t i = 0; i < ids.size(); i++)
		{
			Vegetation &v = vegetation[ids[i]];

			for (int j = 0; j < v.density; j++)
			{
				float r01 = Random::Float();
				float rMinusOne_One = r01 * 2.0f - 1.0f;		// Convert from [0,1] to [-1,1]

				float angle = rMinusOne_One * 6.283186f;

				float x = Random::Float() * vegBrushRadius * glm::cos(angle);
				float z = Random::Float() * vegBrushRadius * glm::sin(angle);

				glm::vec3 position = glm::vec3(intersectionPoint.x + x, 0.0f, intersectionPoint.z + z);

				/*if (obstacleMap[(int)position.z * obstacleMapWidth + (int)position.x] < 230)
					continue;*/

				if (GetNormalAt(position.x, position.z).y < v.maxSlope)		// The closer the slope is to 0 the more steep is the surface
					continue;

				//float height = GetHeightAt((int)position.x, (int)position.z);
				float height = GetExactHeightAt(position.x, position.z);
				position.y = height + v.heightOffset;

				float rotY = (Random::Float() * 2.0f - 1.0f) * 6.283186f;		// Convert from [0,1] to [-1,1] and then multiply by 2pi to have a 0 to 360 random rotation
				float scale = Random::Float() * (v.maxScale - v.minScale) + v.minScale;

				glm::mat4 instanceMatrix = glm::translate(glm::mat4(1.0f), position);
				instanceMatrix = glm::rotate(instanceMatrix, rotY, glm::vec3(0.0f, 1.0f, 0.0f));
				instanceMatrix = glm::scale(instanceMatrix, glm::vec3(scale));

				if (v.count < v.capacity)
				{
					vegetationInstData[v.offset + v.count] = { instanceMatrix };
				}
				else
				{
					vegetationInstData.insert(vegetationInstData.begin() + v.offset + v.count, { instanceMatrix });		// Insert when this veg type begins with v.offset and at the end of all other instances of this veg type (useful for undo)
					v.capacity++;
				}
				
				v.count++;			
			}

			// Update the offsets
			for (size_t j = ids[i] + 1; j < vegetation.size(); j++)
			{
				vegetation[j].offset = vegetation[j - 1].offset + vegetation[j - 1].capacity;
			}
		}
	}

	void Terrain::UndoVegetationPaint(const std::vector<int> &ids)
	{
		// To undo we reduce the count of a vegetation type and update the model instance info. It will render less instances and
		// there's no need to erase elements from the vegetationInstData. When adding a new instance of a vegetation type we just replace
		// 

		for (size_t i = 0; i < ids.size(); i++)
		{
			Vegetation &v = vegetation[ids[i]];
			v.count--;
		}
	}

	void Terrain::RedoVegetationPaint(const std::vector<int> &ids)
	{
		for (size_t i = 0; i < ids.size(); i++)
		{
			Vegetation &v = vegetation[ids[i]];
			v.count++;
		}
	}

	void Terrain::ReseatVegetation()
	{
		struct data
		{
			unsigned int offset;
			unsigned int index;
		};
		std::vector<data> dat;
		for (size_t i = 0; i < vegetation.size(); i++)
		{
			if (vegetation[i].capacity > 0)				// Check if count is more than 0 otherwise it can crash in the for below because we might have added the vegetation but not placed any
				dat.push_back({ vegetation[i].offset, static_cast<unsigned int>(i) });
		}

		unsigned int datIndex = 1;		// Start at 1 so we don't skip the first veg model
		//size_t size = dat.size();
		for (size_t i = 0; i < vegetationInstData.size(); i++)
		{
			if (i + 1 > dat[datIndex].offset)		// i+1 otherwise the last instance of a veg type doesn't get updated
				datIndex++;

			glm::vec4 &v = vegetationInstData[i].modelMatrix[3];
			v = glm::vec4(v.x, GetHeightAt((int)v.x, (int)v.z) + vegetation[dat[datIndex - 1].index].heightOffset, v.z, 1.0f);
		}
	}

	void Terrain::CreateVegColliders()
	{
		if (vegColCreated)
			return;

		closestColliders.resize(maxColliders);

		for (size_t i = 0; i < maxColliders; i++)
		{
			Collider *col = game->GetPhysicsManager().AddBoxCollider(btVector3(i * 2.0f, 0.0f, 0.0f), btVector3(0.4f, 2.0f, 0.4f));		// Use colliders or kinematic rigid bodies?
			closestColliders[i] = { col, 100000.0f, glm::vec3() };
		}

		vegColCreated = true;
	}

	void Terrain::SetVegCollidersRefPoint(const glm::vec3 &point)
	{
		collidersRefPoint = point;
	}

	void Terrain::SetMaterial(const std::string &path)
	{
		MaterialInstance *mat = renderer->CreateMaterialInstance(game->GetScriptManager(), path, terrainInputDescs);

		for (size_t i = 0; i < matInstance->textures.size(); i++)
		{
			if (matInstance->textures[i])
				matInstance->textures[i]->RemoveReference();
		}

		renderer->RemoveMaterialInstance(matInstance);	

		matInstance = mat;

		SetHeightmap(matInstance->textures[0]->GetPath());
	}

	void Terrain::SetHeightmap(const std::string &path)
	{
		Texture *heightmap = matInstance->textures[0];			// The heightmap is always the first element

		if (path != matInstance->textures[0]->GetPath())		// Only reload if it's not the same texture
		{
			// TODO: Texture is not being deleted
			TextureParams params = heightmap->GetTextureParams();
			renderer->RemoveTexture(heightmap);
			// We can now use the heightmap, which is now null, to change the first element of the vector to the new heightmap
			heightmap = renderer->CreateTexture2D(path, params, true);
			matInstance->textures[0] = heightmap;
			renderer->UpdateMaterialInstance(matInstance);
		}

		if (!heightmap)
		{
			std::cout << "Error! Failed to load terrain heightmap\n";
			return;
		}

		unsigned int width = heightmap->GetWidth();
		unsigned int height = heightmap->GetHeight();

		if (width != height || width == 0 || height == 0)
		{
			std::cout << "Error! Heightmap width and height must be equal and different from zero\n";
			return;
		}

		unsigned char *data = heightmap->GetData();

		if (!data)
		{
			std::cout << "Error! Heightmap texture data is null\n";
			return;
		}

		// Reallocate the heights if the res is not the same
		if ((unsigned int)resolution != width || !heights)
		{
			resolution = width;
			updateMaterialUBO = true;

			// If stat. needed because at the first time heights is not allocated and resolution is 0
			if (heights)
			{
				delete[] heights;
				heights = nullptr;
			}

			heights = new float[resolution * resolution];

			if (!heights)
			{
				std::cout << "Error! Failed to reallocate terrain heights\n";
				return;
			}

			for (int z = 0; z < resolution; z++)
			{
				for (int x = 0; x < resolution; x++)
				{
					heights[z * resolution + x] = (float)data[z * resolution + x];
				}
			}
			SmoothTerrain();

			terrainShapeID = game->GetPhysicsManager().AddTerrainCollider(terrainShapeID, resolution, heights, 256.0f);
		}
		else
		{
			for (int z = 0; z < resolution; z++)
			{
				for (int x = 0; x < resolution; x++)
				{
					heights[z * resolution + x] = (float)data[z * resolution + x];
				}
			}

			SmoothTerrain();
		}

		CreateMesh();

		// Needs to be recalculated when we change heightmap
		CreateQuadtree();
		CalculateLODVisRanges();

		int nChannels;

		// Try to load the obstacle map
		obstacleMap = stbi_load("Data/Textures/obstacle_map.png", &obstacleMapWidth, &obstacleMapHeight, &nChannels, STBI_grey);

		if (!obstacleMap)
		{
			std::cout << "Error! Failed to load obstacle map texture: Data/Textures/obstacle_map.png" << "\n";
			return;
		}

		else if (obstacleMapWidth != resolution && obstacleMapHeight != resolution)
		{
			std::cout << "Error! Obstacle map must have the same dimensions as the terrain\n Terrain size: " << resolution << "\nObstacle map size: " << obstacleMapWidth << "\n";
			stbi_image_free(obstacleMap);
			obstacleMap = nullptr;
			return;
		}
	}

	void Terrain::SmoothTerrain()
	{
		for (int z = 0; z < resolution; z++)
		{
			for (int x = 0; x < resolution; x++)
			{
				float up = GetHeightAt(x, z - 1);
				float down = GetHeightAt(x, z + 1);
				float left = GetHeightAt(x - 1, z);
				float right = GetHeightAt(x + 1, z);
				float center = GetHeightAt(x, z);

				heights[z * resolution + x] = (center + up + down + left + right) * 0.2f;
			}
		}
	}

	void Terrain::CreateMesh()
	{
		if (terrainInstancingBuffer)
			return;

		unsigned int nodeRes = 16;

		std::vector<VertexPOS2D_UV> vertices((nodeRes + 1) * (nodeRes + 1));

		int index = 0;

		for (unsigned int z = 0; z <= nodeRes; z++)
		{
			for (unsigned int x = 0; x <= nodeRes; x++)
			{
				VertexPOS2D_UV v;
				v.posuv = glm::vec4((float)x, (float)z, (float)x / nodeRes, (float)z / nodeRes);
				vertices[index++] = v;
			}
		}

		std::vector<unsigned short> indices(nodeRes * nodeRes * 6);

		index = 0;

		for (unsigned int z = 0; z < nodeRes; z++)
		{
			for (unsigned int x = 0; x < nodeRes; x++)
			{
				unsigned short tl = z * ((unsigned short)nodeRes + 1) + x;			// Resolution + 1 otherwise with would end up with resolution - 1 rows
				unsigned short tr = tl + 1;
				unsigned short bl = (z + 1) * ((unsigned short)nodeRes + 1) + x;
				unsigned short br = bl + 1;

				indices[index++] = tl;
				indices[index++] = bl;
				indices[index++] = br;

				indices[index++] = tl;
				indices[index++] = br;
				indices[index++] = tr;
			}
		}

		Buffer *vb = renderer->CreateVertexBuffer(vertices.data(), vertices.size() * sizeof(VertexPOS2D_UV), BufferUsage::STATIC);
		Buffer *ib = renderer->CreateIndexBuffer(indices.data(), indices.size() * sizeof(unsigned short), BufferUsage::STATIC);

		terrainInstancingBuffer = renderer->CreateVertexBuffer(nullptr, resolution * sizeof(TerrainInstanceData), BufferUsage::DYNAMIC);

		const std::vector<Buffer*> vbs = { vb, terrainInstancingBuffer };

		mesh = {};
		mesh.vao = renderer->CreateVertexArray(terrainInputDescs.data(), terrainInputDescs.size(), vbs, ib);
		mesh.indexCount = indices.size();
		mesh.instanceCount = 0;
		mesh.instanceOffset = 0;
		mesh.indexOffset = 0;
	}

	void Terrain::CreateQuadtree()
	{
		int topNodeSize = meshSize;
		int totalNodeCount = 0;

		std::cout << "Root node size: " << topNodeSize << "\n";

		for (int i = 0; i < lodLevels; i++)
		{
			if (i != 0)
			{
				topNodeSize *= 2;
			}

			int nodeCountX = (int)((resolution - 1) / topNodeSize + 1);
			int nodeCountY = (int)((resolution - 1) / topNodeSize + 1);

			totalNodeCount += nodeCountX * nodeCountY;
		}

		std::cout << "Quadtree node size: " << topNodeSize << "\n";
		std::cout << "Total node count: " << totalNodeCount << "\n";

		int topNodeCount = (int)((resolution - 1) / topNodeSize + 1);
		std::cout << "Top node count:" << topNodeCount << "\n";
		std::cout << "Top node size:" << topNodeSize << "\n";

		// Used when the new resolution is lower than the older one, to delete the nodes which will not be used
		if ((int)nodes.size() > topNodeCount)
		{
			for (size_t z = 0; z < nodes.size(); z++)
			{
				for (size_t x = 0; x < nodes[z].size(); x++)
				{
					if ((int)x >= topNodeCount || (int)z >= topNodeCount)
					{
						delete nodes[z][x];
						nodes[z][x] = nullptr;
					}
				}
			}
		}

		if (nodes.size() != (size_t)topNodeCount)
			nodes.resize(topNodeCount);

		for (int z = 0; z < topNodeCount; z++)
		{
			if (nodes[z].size() != (size_t)topNodeCount)
				nodes[z].resize(topNodeCount);

			for (int x = 0; x < topNodeCount; x++)
			{
				if (!nodes[z][x])
					nodes[z][x] = new TerrainNode(x * topNodeSize, z * topNodeSize, topNodeSize, lodLevels - 1, heights, resolution);
				else
					nodes[z][x]->RecalculateNode(x * topNodeSize, z * topNodeSize, topNodeSize, lodLevels - 1, heights, resolution);
			}
		}

		int sizeInMemory = totalNodeCount * sizeof(TerrainNode);
		std::cout << "CDLOD quadtree created, size in memory: " << sizeInMemory / 1024.0f << "kb\n";
	}

	void Terrain::CalculateLODVisRanges()
	{
		float lodLevelDistRatio = 2.0f;
		float lodNear = 0.0f;
		//float lodFar = 100.0f;

		float totalDetailBalance = 0.0f;
		float curDetailBalance = 1.0f;

		for (int i = 0; i < lodLevels; i++)
		{
			totalDetailBalance += curDetailBalance;
			curDetailBalance *= lodLevelDistRatio;
		}

		float sect = (lodFar - lodNear) / totalDetailBalance;
		float prevPos = lodNear;
		curDetailBalance = 1.0f;
		for (int i = 0; i < lodLevels; i++)
		{
			lodVisRanges[i] = prevPos + sect * curDetailBalance;
			prevPos = lodVisRanges[i];
			curDetailBalance *= lodLevelDistRatio;
		}
	}

	void Terrain::SetHeightScale(float scale)
	{
		heightScale = scale;
		updateMaterialUBO = true;
	}

	void Terrain::SetBrushRadius(float radius)
	{
		brushRadius = radius;
	}

	void Terrain::SetBrushStrength(float strength)
	{
		brushStrength = strength;
	}

	void Terrain::SetVegetationBrushRadius(float radius)
	{
		vegBrushRadius = radius;
		updateMaterialUBO = true;
	}

	float Terrain::GetHeightAt(int x, int z)
	{
		if (x < 0 || x >= resolution || z < 0 || z >= resolution)
			return 0.0f;

		return heights[z * resolution + x];
	}

	float Terrain::Barycentric(const glm::vec3 &p1, const glm::vec3 &p2, const glm::vec3 &p3, const glm::vec2 &pos)
	{
		float det = (p2.z - p3.z) * (p1.x - p3.x) + (p3.x - p2.x) * (p1.z - p3.z);
		float l1 = ((p2.z - p3.z) * (pos.x - p3.x) + (p3.x - p2.x) * (pos.y - p3.z)) / det;
		float l2 = ((p3.z - p1.z) * (pos.x - p3.x) + (p1.x - p3.x) * (pos.y - p3.z)) / det;
		float l3 = 1.0f - l1 - l2;

		return  l1 * p1.y + l2 * p2.y + l3 * p3.y;
	}

	float Terrain::GetExactHeightAt(float x, float z)
	{
		if (x < 0 || x >= resolution || z < 0 || z >= resolution)
			return 0.0f;

		float gridSquareSize = 1.0f;
		int gridX = (int)x;
		int gridZ = (int)z;

		float xCoord = std::fmod(x, gridSquareSize);
		float zCoord = std::fmod(z, gridSquareSize);

		if (zCoord <= (1 - xCoord))
		{
			glm::vec3 p1 = glm::vec3(0.0f, GetHeightAt(gridX, gridZ), 0.0f);
			glm::vec3 p2 = glm::vec3(1.0f, GetHeightAt(gridX + 1, gridZ), 0.0f);
			glm::vec3 p3 = glm::vec3(0.0f, GetHeightAt(gridX, gridZ + 1), 1.0f);
			return Barycentric(p1, p2, p3, glm::vec2(xCoord, zCoord));
		}
		else
		{
			glm::vec3 p1 = glm::vec3(1.0f, GetHeightAt(gridX + 1, gridZ), 0.0f);
			glm::vec3 p2 = glm::vec3(1.0f, GetHeightAt(gridX + 1, gridZ + 1), 1.0f);
			glm::vec3 p3 = glm::vec3(0.0f, GetHeightAt(gridX, gridZ + 1), 1.0f);
			return Barycentric(p1, p2, p3, glm::vec2(xCoord, zCoord));
		}

		return 0.0f;
	}

	glm::vec3 Terrain::GetNormalAtFast(int x, int z)
	{
		float up = GetHeightAt(x, z - 1);
		float down = GetHeightAt(x, z + 1);
		float left = GetHeightAt(x - 1, z);
		float right = GetHeightAt(x + 1, z);

		glm::vec3 n = glm::vec3(left - right, 2.0f, up - down);

		return glm::normalize(n);
	}

	glm::vec3 Terrain::GetNormalAt(float x, float z)
	{
		float up = GetExactHeightAt(x, z - 1);
		float down = GetExactHeightAt(x, z + 1);
		float left = GetExactHeightAt(x - 1, z);
		float right = GetExactHeightAt(x + 1, z);

		glm::vec3 n = glm::vec3(left - right, 2.0f, up - down);

		return glm::normalize(n);
	}

	bool Terrain::IntersectTerrain(const glm::vec3 &rayOrigin, const glm::vec3 &rayDir, glm::vec3 &intersectionPoint)
	{
		float delta = 0.25f;
		float mint = 0.1f;		// cam near plane
		float maxt = 200.0f;	// Max calc distance

		for (float t = mint; t < maxt; t += delta)
		{
			intersectionPoint = rayOrigin + rayDir * t;

			//float height = GetBetterHeight(interPoint.x, interPoint.z);
			float height = GetHeightAt((int)intersectionPoint.x, (int)intersectionPoint.z);

			if (intersectionPoint.y < height)												// If we are below the terrain...
			{
				intersectionPoint = ((rayOrigin + rayDir * t) + (rayOrigin + rayDir * (t - delta))) / 2.0f;
				//intersectionPoint.y = height;
				return true;
			}
			//delta = 0.5f * t;
		}

		//updateMaterialUBO = true;
		return false;
	}

	bool Terrain::InBounds(int x, int z)
	{
		if (x < 0 && x >= resolution && z < 0 && z >= resolution)
			return false;

		return true;
	}

	void Terrain::AddVegInstanceBufferToMesh(const Mesh &mesh, int lod)
	{
		if (lod == 0)
			mesh.vao->AddVertexBuffer(vegInstancingBufferLOD0);
		else if (lod == 1)
			mesh.vao->AddVertexBuffer(vegInstancingBufferLOD1);
		else if (lod == 2)
			mesh.vao->AddVertexBuffer(vegInstancingBufferLOD2);
	}

	void Terrain::LoadVegetationFile(const std::string &vegPath)
	{
		if (vegPath.length() == 0)		// Possible if eg: we're adding the terrain for the first time and we've never added vegetation
			return;

		// Read the config file
		std::ifstream file(vegPath);

		if (!file.is_open())
		{
			std::cout << "Error! Failed to open terrain vegetation file: " << vegPath << "\n";
			return;
		}

		std::string line;
		Vegetation v = {};
		v.maxScale = 1.0f;
		v.minScale = 1.0f;
		v.maxSlope = 0.0f;
		v.modelLOD1 = nullptr;
		v.modelLOD2 = nullptr;

		while (std::getline(file, line))
		{
			if (line.substr(0, 4) == "mat=")
				v.matnames.push_back(line.substr(4));
			else if (line.substr(0, 8) == "density=")
				v.density = std::stoi(line.substr(8));
			else if (line.substr(0, 9) == "maxSlope=")
				v.maxSlope = std::stof(line.substr(9));
			else if (line.substr(0, 9) == "minScale=")
				v.minScale = std::stof(line.substr(9));
			else if (line.substr(0, 9) == "maxScale=")
				v.maxScale = std::stof(line.substr(9));
			else if (line.substr(0, 8) == "lodDist=")
				v.lodDist = std::stof(line.substr(8));
			else if (line.substr(0, 9) == "lod1Dist=")
				v.lod1Dist = std::stof(line.substr(9));
			else if (line.substr(0, 9) == "lod2Dist=")
				v.lod2Dist = std::stof(line.substr(9));
			else if (line.substr(0, 13) == "heightOffset=")
				v.heightOffset = std::stof(line.substr(13));
			else if (line.substr(0, 7) == "genCol=")
				v.generateColliders = (bool)std::stoi(line.substr(7));
			else if (line.substr(0, 7) == "genObs=")
				v.generateObstacles = (bool)std::stoi(line.substr(7));
			else if (line.substr(0, 10) == "modelLOD1=")
			{
				//v.modelLOD1 = game->GetModelManager().LoadModel(line.substr(10), false, true, v.matnames);					// Add model from custom format
			}
			else if (line.substr(0, 10) == "modelLOD2=")
			{
				//v.modelLOD2 = game->GetModelManager().LoadModel(line.substr(10), false, true, v.matnames);					// Add model from custom format
			}
			else if (line.substr(0, 6) == "model=")
			{
				//v.model = ResourcesLoader::LoadModel(line.substr(6), false, true, v.matnames, game->GetScriptManager());

				// Add model from custom format
				//v.model = game->GetModelManager().LoadModel(line.substr(6), false, true, v.matnames);		// Don't use true for instancing, we use a separate buffer
				vegetation.push_back(v);
				v.matnames.clear();
				v.modelLOD1 = nullptr;
				v.modelLOD2 = nullptr;
			}
			
		}

		file.close();

		Serializer s(game->GetFileManager());
		s.OpenForReading(vegPath + "a");		// Veg cfg file is stored as vegetation_scenename.dat	and the vegetation data file (pos, rot...) is stored as vegetation_scenename.data, that's why the +"a"

		if (s.IsOpen())
		{
			unsigned int size = 0;
			unsigned int count = 0;

			s.Read(size);
			for (unsigned int i = 0; i < size; i++)
			{
				unsigned int c = 0;
				s.Read(c);

				vegetation[i].count = c;
				vegetation[i].capacity = c;
				count += c;
			}

			for (unsigned int i = 1; i < size; i++)
			{
				vegetation[i].offset = vegetation[i - 1].offset + vegetation[i - 1].capacity;
			}

			vegetationInstData.resize(count);

			glm::vec3 pos;
			float rotYEuler = 0.0f;
			float scale = 0.0f;
			for (size_t i = 0; i < count; i++)
			{
				s.Read(pos);
				s.Read(rotYEuler);
				s.Read(scale);
				vegetationInstData[i].modelMatrix = glm::translate(glm::mat4(1.0f), pos);
				vegetationInstData[i].modelMatrix = glm::rotate(vegetationInstData[i].modelMatrix, glm::radians(rotYEuler), glm::vec3(0.0f, 1.0f, 0.0f));
				vegetationInstData[i].modelMatrix = glm::scale(vegetationInstData[i].modelMatrix, glm::vec3(scale));
			}
		}

		s.Close();


		CreateVegInstanceBuffer();

		// Add the vegetation instancing buffers to the meshes
		for (size_t i = 0; i < vegetation.size(); i++)
		{
			const Vegetation &v = vegetation[i];
			const std::vector<MeshMaterial> &meshesAndMaterials = v.model->GetMeshesAndMaterials();

			for (size_t j = 0; j < meshesAndMaterials.size(); j++)
			{
				AddVegInstanceBufferToMesh(meshesAndMaterials[j].mesh, 0);
			}

			if (v.modelLOD1)
			{
				const std::vector<MeshMaterial> &meshesAndMaterials = v.modelLOD1->GetMeshesAndMaterials();
				for (size_t j = 0; j < meshesAndMaterials.size(); j++)
				{
					AddVegInstanceBufferToMesh(meshesAndMaterials[j].mesh, 1);
				}
			}

			if (v.modelLOD2)
			{
				const std::vector<MeshMaterial> &meshesAndMaterials = v.modelLOD2->GetMeshesAndMaterials();
				for (size_t j = 0; j < meshesAndMaterials.size(); j++)
				{
					AddVegInstanceBufferToMesh(meshesAndMaterials[j].mesh, 2);
				}
			}
		}
	}

	void Terrain::CreateVegInstanceBuffer()
	{
		// We use a single buffer for instancing for all the vegetation instead of one buffer per mesh
#ifdef EDITOR
		vegInstancingBufferLOD0 = renderer->CreateVertexBuffer(nullptr, 3000 * sizeof(ModelInstanceData), BufferUsage::DYNAMIC);
		vegInstancingBufferLOD1 = renderer->CreateVertexBuffer(nullptr, 3000 * sizeof(ModelInstanceData), BufferUsage::DYNAMIC);
		vegInstancingBufferLOD2 = renderer->CreateVertexBuffer(nullptr, 4500 * sizeof(ModelInstanceData), BufferUsage::DYNAMIC);
#else
		if (vegetationInstData.size() > 0)
		{
			vegInstancingBufferLOD0 = renderer->CreateVertexBuffer(nullptr, vegetationInstData.size() * sizeof(ModelInstanceData), BufferUsage::DYNAMIC);
			vegInstancingBufferLOD1 = renderer->CreateVertexBuffer(nullptr, vegetationInstData.size() * sizeof(ModelInstanceData), BufferUsage::DYNAMIC);
			vegInstancingBufferLOD2 = renderer->CreateVertexBuffer(nullptr, vegetationInstData.size() * sizeof(ModelInstanceData), BufferUsage::DYNAMIC);
		}
#endif
	}

	void Terrain::Save(const std::string &folder, const std::string &sceneName)
	{
		std::ofstream file(folder + "terrain_" + sceneName + ".dat");

		if (!file.is_open())
		{
			std::cout << "Error! Failed to save terrain!\n";
			return;
		}

		file << "mat=" << matInstance->path << '\n';

		if (vegetation.size() > 0)
		{
			file << "veg=" << folder << "vegetation_" << sceneName << ".dat\n";		// Replace with path to vegetation file
		}

		//file.close();

		std::ofstream vegFile(folder + "vegetation_" + sceneName + ".dat");

		if (!vegFile.is_open())
		{
			std::cout << "Error! Failed to save terrain!\n";
			return;
		}

		for (size_t i = 0; i < vegetation.size(); i++)
		{
			const Vegetation &v = vegetation[i];

			const std::vector<MeshMaterial> &meshesAndMaterials = v.model->GetMeshesAndMaterials();

			for (size_t j = 0; j < meshesAndMaterials.size(); j++)
			{
				vegFile << "mat=" << meshesAndMaterials[j].mat->path << '\n';
			}
			vegFile << "maxSlope=" << v.maxSlope << '\n';
			vegFile << "minScale=" << v.minScale << '\n';
			vegFile << "maxScale=" << v.maxScale << '\n';
			vegFile << "density=" << v.density << '\n';
			vegFile << "lodDist=" << v.lodDist << '\n';
			vegFile << "lod1Dist=" << v.lod1Dist << '\n';
			vegFile << "lod2Dist=" << v.lod2Dist << '\n';
			vegFile << "heightOffset=" << v.heightOffset << '\n';
			vegFile << "genCol=" << v.generateColliders << '\n';
			vegFile << "genObs=" << v.generateObstacles << '\n';

			if (v.modelLOD1)
				vegFile << "modelLOD1=" << v.modelLOD1->GetPath() << '\n';
			if (v.modelLOD2)
				vegFile << "modelLOD2=" << v.modelLOD2->GetPath() << '\n';

			vegFile << "model=" << v.model->GetPath() << '\n';		
		}

		//vegFile.close();

		Serializer s(game->GetFileManager());
		s.OpenForWriting();
		s.Write(static_cast<unsigned int>(vegetation.size()));
		for (size_t i = 0; i < vegetation.size(); i++)
		{
			s.Write(vegetation[i].count);
		}

		unsigned int vegIndex = 0;
		unsigned int counter = 0;
		for (size_t i = 0; i < vegetationInstData.size(); i++)
		{
			if (i + 1 > vegetation[vegIndex].count+vegetation[vegIndex].offset)
			{
				i += vegetation[vegIndex].capacity - vegetation[vegIndex].count;
				
				vegIndex++;
				if (vegetation[vegIndex].count == 0)
					i += vegetation[vegIndex].capacity;

				if (i >= vegetationInstData.size())
					break;
			}

			glm::quat q = glm::quat_cast(vegetationInstData[i].modelMatrix);
			float rotYEuler = glm::degrees(glm::eulerAngles(q).y);
			float scale = glm::length(glm::vec3(vegetationInstData[i].modelMatrix[0]));		// Because the scale is equal on all axis just take the length on the first one because it's equal on the others

			s.Write(glm::vec3(vegetationInstData[i].modelMatrix[3]));
			s.Write(rotYEuler);
			s.Write(scale);
			counter++;
		}

		Log::Print(LogLevel::LEVEL_INFO, "Saved %d vegetation instances\n", counter);

		s.Save(folder + "vegetation_" + sceneName + ".data");
		s.Close();
	}
}
