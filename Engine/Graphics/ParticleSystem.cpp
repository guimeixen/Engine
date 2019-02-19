#include "ParticleSystem.h"

#include "Buffers.h"
#include "VertexArray.h"
#include "Material.h"
#include "Renderer.h"
#include "Buffers.h"
#include "ResourcesLoader.h"
#include "Program\Random.h"
#include "Game\Game.h"

#include "include\glm\gtc\matrix_transform.hpp"
#include "include\glm\gtc\type_ptr.hpp"

#include <iostream>

namespace Engine
{
	ParticleSystem::ParticleSystem()
	{
		lastUsedParticle = 0;
		accumulator = 0.0f;
		velocityLow = glm::vec3(0.0f);
		velocityHigh = glm::vec3(1.0f);
		useRandomVelocity = false;
		useAtlas = false;
		startVelocity = glm::vec3(0.0f, 0.5f, 0.0f);
		aabb.min = glm::vec3(-0.5f);
		aabb.max = glm::vec3(0.5f);
		startSize = 1.0f;
		startColor = glm::vec4(1.0f, 1.0f, 1.0f, 0.0f);		// Hack to get transition from transparent to opaque to transparent working. Add in editor!
		startLifeTime = 1.0f;

		atlasInfo.nColumns = 1;
		atlasInfo.nRows = 1;
		atlasInfo.cycles = 1.0f;

		center = glm::vec3(0.0f);

		emissionBox = glm::vec3(1.0f);
		emissionRadius = 1.0f;
		emissionShape = EmissionShape::BOX;
	}

	ParticleSystem::~ParticleSystem()
	{
		if (quadMesh.vao)
		{
			delete quadMesh.vao;
			quadMesh.vao = nullptr;
		}

		renderer->RemoveMaterialInstance(matInstance);
	}

	void ParticleSystem::Init(Game *game, const std::string &matPath)
	{
		renderer = game->GetRenderer();

		if (atlasInfo.nColumns > 1 || atlasInfo.nRows > 1)
			SetAtlasInfo(atlasInfo.nColumns, atlasInfo.nRows, atlasInfo.cycles);

		VertexPOS2D_UV vertices[4];
		vertices[0] = { glm::vec4(-1.0f, 1.0f,	0.0f, 1.0f) };
		vertices[1] = { glm::vec4(-1.0f, -1.0f,	0.0f, 0.0f) };
		vertices[2] = { glm::vec4(1.0f, -1.0f,	1.0f, 0.0f) };
		vertices[3] = { glm::vec4(1.0f, 1.0f,	1.0f, 1.0f) };

		unsigned short indices[] = { 0,1,2, 0,2,3 };

		Buffer *vb = renderer->CreateVertexBuffer(vertices, sizeof(vertices), BufferUsage::STATIC);
		instanceVB = renderer->CreateVertexBuffer(nullptr, 100 * sizeof(PSInstanceData), BufferUsage::DYNAMIC);		// Allocate space for 100 particles. Allocate more when in editor and allocate just enough when in game
		Buffer *ib = renderer->CreateIndexBuffer(indices, sizeof(indices), BufferUsage::STATIC);

		std::vector<Buffer*> vbs = { vb, instanceVB };

		VertexAttribute posUv = {};
		posUv.count = 4;
		posUv.offset = 0;
		posUv.vertexAttribFormat = VertexAttributeFormat::FLOAT;

		VertexAttribute instance[3] = {};
		instance[0].count = 4;
		instance[1].count = 4;
		instance[2].count = 4;

		instance[0].offset = 0;
		instance[1].offset = 4 * sizeof(float);
		instance[2].offset = 8 * sizeof(float);

		std::vector<VertexInputDesc> descs(2);
		descs[0].stride = 4 * sizeof(float);
		descs[0].attribs = { posUv };
		descs[0].instanced = false;

		descs[1].stride = 12 * sizeof(float);
		descs[1].attribs = { instance[0],instance[1],instance[2] };
		descs[1].instanced = true;

		quadMesh = {};
		quadMesh.vao = renderer->CreateVertexArray(descs.data(), descs.size(), vbs, ib);
		quadMesh.indexCount = 6;
		quadMesh.instanceCount = 0;
		quadMesh.indexOffset = 0;
		quadMesh.instanceOffset = 0;
		quadMesh.vertexOffset = 0;

		matInstance = renderer->CreateMaterialInstance(game->GetScriptManager(), matPath, descs);
	}

	void ParticleSystem::Create(int maxParticles)
	{
		if (maxParticles == 0)
			return;

		this->maxParticles = maxParticles;
		particles.resize(maxParticles);

		if (particles.size() > 0)
		{
			RespawnParticle(particles[0]);		// Spawn one particle so they get update initially
		}
	}

	void ParticleSystem::Update(float dt)
	{
		// If we're not looping, this ps has been playing more than it's duration and if there aren't particles alive only then we can stop playing
		if (!isLooping && timePlaying >= duration && !particlesAlive)
			Stop();

		if (playing || particlesAlive)
		{
			// Reset the aabb because if the particle's were to increase in size and then decrease the aabb max would retain the largest size/would not update to the new size. Similar thing for min
			// But we have to take into account if there are any particles alive, otherwise the aabb would bet set to 10000/-10000 and the update would never be called again (because it would not be in the frustum)
			// So store the previous min/max and check after the for loop if there are no particles alive to put the aabb back to what it was
			glm::vec3 prevMin = aabb.min;
			glm::vec3 prevMax = aabb.max;
			
			aabb.min = glm::vec3(10000.0f, 10000.0f, 10000.0f);
			aabb.max = glm::vec3(-10000.0f, -10000.0f, -10000.0f);

			particlesAlive = false;

			for (unsigned int i = 0; i < maxParticles; i++)
			{
				Particle &p = particles[i];

				p.life -= dt / startLifeTime;

				float lifeFactor = (startLifeTime - p.life) / startLifeTime;			// Goes from 0 to 1 over the particle's life time

				if (p.life > 0.0f)
				{
					particlesAlive = true;
					// If the particle system is using a texture atlas update the texture coords info
					if (useAtlas)
					{
						//float lifeFactor = (startLifeTime - p.life) / startLifeTime;		// startLifeTime - life, because the particle's life is decreasing we would start at the end of
						lifeFactor *= atlasInfo.cycles;																	// the atlas and this makes it so that we start at the beginning of the atlas

						int stageCount = atlasInfo.nColumns * atlasInfo.nRows;
						float atlasProgression = (lifeFactor * stageCount);

						int index1 = (int)glm::floor(atlasProgression);
						int index2 = index1 < stageCount - 1 ? index1 + 1 : index1;

						p.blendFactor = std::fmodf(atlasProgression, 1.0f);
						p.texOffset1 = SetTextureOffset(index1);
						p.texOffset2 = SetTextureOffset(index2);
					}

					// Update position
					if (limitVelocity)
					{
						if (p.velocity.x > speedLimit)
						{
							p.velocity.x = p.velocity.x * speedLimit / dampen * (1.0f - lifeFactor);
							if (p.velocity.x < 0.0f)
								p.velocity.x = 0.0f;
						}
						if (p.velocity.y > speedLimit)
						{
							p.velocity.y = p.velocity.y * speedLimit / dampen * (1.0f - lifeFactor);
							if (p.velocity.y < 0.0f)
								p.velocity.y = 0.0f;
						}
						if (p.velocity.z > speedLimit)
						{
							p.velocity.z = p.velocity.z * speedLimit / dampen * (1.0f - lifeFactor);
							if (p.velocity.z < 0.0f)
								p.velocity.z = 0.0f;
						}
					}

					p.velocity.y += -9.8f * gravityModifier * dt;

					p.position += p.velocity * dt;
					
					aabb.max = glm::max(aabb.max, p.position + worldPos);
					aabb.min = glm::min(aabb.min, p.position + worldPos);

					//p.zRotation += dt;

					if (fadeAlphaOverLifetime)
					{
						// Calculate alpha between 0 to 1 and to 0 again
						// Divide the cur particle life by startlifetime to get the value in [0,1]. Because the particle's life starts at 1 and goes to 0, so we need to invert it so it goes from 0 to 1
						// Multiply it by 2pi so we cover the whole cos range:   x:0 -> y:0     x:pi -> y:1     x:2pi -> y:0
						float lifetime01 = (1.0f - p.life / startLifeTime) * 6.28f;

						// Plot it in desmos calc to see the graph  (-cos x + 1) * 0.5
						// It goes from 0 to 1 and then to 0
						p.color.w = (-glm::cos(lifetime01) + 1.0f) * 0.5f;
					}
				}
			}

			if (particlesAlive == false)
			{
				aabb.min = prevMin;
				aabb.max = prevMax;
			}


			// Only spawn particles if we're below this particle's system duration or we're looping
			if (timePlaying <= duration || isLooping)
			{
				accumulator += dt;							// This line and the while are used to spawn newParticles (emission) per second and not per frame
				float denom = 1.0f / emission;

				while (accumulator > denom)
				{
					int unusedParticle = FirstUnusedParticle();
					if (unusedParticle == 0 && particles[0].life <= 0.0f)		// This is used in the case that there is only one particle to let its life reach 0
					{
						RespawnParticle(particles[0]);
					}
					else if (unusedParticle > 0)
					{
						RespawnParticle(particles[unusedParticle]);
					}
					accumulator -= denom;
				}
			}		
			
			timePlaying += dt;
			//if (useGlobalRotation && useRotation)
				//zRotation += dt;

			//std::cout << "x: " << aabb.min.x << "y: " << aabb.min.y << "z: " << aabb.min.z << '\n';
			//std::cout << "x: " << aabb.max.x << "y: " << aabb.max.y << "z: " << aabb.max.z << '\n';
		}
	}

	bool ParticleSystem::PrepareRender(const glm::mat4 &transform)
	{
		if (!playing)
			return false;

		quadMesh.instanceOffset = instanceData.size();

		glm::mat4 m = transform;
		m[0] = glm::normalize(m[0]);
		m[1] = glm::normalize(m[1]);
		m[2] = glm::normalize(m[2]);

		glm::mat4 t = glm::mat4(1.0f);

		for (unsigned int i = 0; i < maxParticles; i++)
		{
			const Particle &p = particles[i];

			if (p.life > 0.0f)
			{
				PSInstanceData inst;
				//inst.posBlendFactor = glm::vec4(particles[i].position + worldPos, particles[i].blendFactor);
				
				t = glm::mat4(1.0f);
				t[3] = glm::vec4(p.position, 1.0f);

				t = m * t;

				inst.posBlendFactor = glm::vec4(t[3].x, t[3].y, t[3].z, p.blendFactor);

				inst.color = p.color;
				inst.texOffsets = glm::vec4(p.texOffset1, p.texOffset2);
				//inst.rotation = glm::vec4(0.0f, 0.0f, particles[i].zRotation, 0.0f);

				instanceData.push_back(inst);
			}
		}

		quadMesh.instanceCount = instanceData.size();

		if (quadMesh.instanceCount > 0)
		{
			/*if (materialUBOChanged)
			{
				glm::vec4 params = glm::vec4(atlasInfo.nColumns, atlasInfo.nRows, startSize, useAtlas);
				matInstance->baseMaterial->GetMaterialUBO()->Update(glm::value_ptr(params), sizeof(glm::vec4), 0);
				materialUBOChanged = false;
			}*/
			instanceVB->Update(instanceData.data(), instanceData.size() * sizeof(PSInstanceData), 0);
			instanceData.clear();
		}

		return true;
	}

	void ParticleSystem::RespawnParticle(Particle &particle)
	{
		// Convert the random float which is in [0,1] to [-1,1]
		float x = Random::Float() * 2.0f - 1.0f;
		float y = Random::Float() * 2.0f - 1.0f;
		float z = Random::Float() * 2.0f - 1.0f;

		// Convert from [-1,1] to [-0.5,0.5] so we have a unit box
		x *= 0.5f;
		y *= 0.5f;
		z *= 0.5f;

		if (emissionShape == EmissionShape::BOX)
		{
			glm::vec3 pos = glm::vec3(x, y, z);
			pos *= emissionBox;

			//particle.position = pos + startPosition + center;
			particle.position = pos + center;
			//particle.position = pos;
		}
		else if (emissionShape == EmissionShape::SPHERE)
		{

		}

		if (useRandomVelocity)
		{
			x = Random::Float() * (velocityHigh.x - velocityLow.x) + velocityLow.x;
			y = Random::Float() * (velocityHigh.y - velocityLow.y) + velocityLow.y;
			z = Random::Float() * (velocityHigh.z - velocityLow.z) + velocityLow.z;

			particle.velocity = glm::vec3(x, y, z);
		}
		else
		{
			particle.velocity = startVelocity;
		}

		particle.life = startLifeTime;
		particle.color = startColor;
	}

	int ParticleSystem::FirstUnusedParticle()
	{
		for (unsigned int i = lastUsedParticle; i < maxParticles; ++i)
		{
			if (particles[i].life <= 0.0f)
			{
				lastUsedParticle = i;
				return i;
			}
		}

		for (int i = 0; i < lastUsedParticle; i++)
		{
			if (particles[i].life <= 0.0f)
			{
				lastUsedParticle = i;
				return i;
			}
		}

		lastUsedParticle = 0;
		return 0;
	}

	glm::vec2 ParticleSystem::SetTextureOffset(int index)
	{
		if (atlasInfo.nColumns <= 0 || atlasInfo.nRows <= 0)
			return glm::vec2(0.0f, 0.0f);

		int column = index % atlasInfo.nColumns;
		int row = index / atlasInfo.nRows;

		return glm::vec2((float)column / atlasInfo.nColumns, (float)row / atlasInfo.nRows);
	}

	void ParticleSystem::SetRandomVelocity(const glm::vec3 &low, const glm::vec3 &high)
	{
		velocityLow = low;
		velocityHigh = high;
		useRandomVelocity = true;
	}

	void ParticleSystem::SetSize(float size)
	{
		startSize = size;
		materialUBOChanged = true;
	}

	void ParticleSystem::SetAtlasInfo(int columns, int rows, float cycles)
	{
		atlasInfo.nColumns = columns;
		atlasInfo.nRows = rows;
		atlasInfo.cycles = cycles;
		useAtlas = true;
		materialUBOChanged = true;
	}

	void ParticleSystem::SetMaxParticles(unsigned int maxParticles)
	{
		if (maxParticles > 100)		// Don't use hardcoded number
			maxParticles = 100;

		this->maxParticles = maxParticles;
		particles.resize(maxParticles);
	}

	void ParticleSystem::SetEmissionBox(const glm::vec3 &box)
	{
		emissionBox = box;
		emissionShape = EmissionShape::BOX;
	}

	void ParticleSystem::SetEmissionRadius(float radius)
	{
		emissionRadius = radius;
		emissionShape = EmissionShape::SPHERE;
	}

	glm::vec4 &ParticleSystem::GetParams()
	{
		params = glm::vec4(atlasInfo.nColumns, atlasInfo.nRows, startSize, static_cast<float>(useAtlas));
		return params;
	}

	void ParticleSystem::Stop()
	{
		playing = false;
	}

	void ParticleSystem::Play()
	{
		if (playing)
			return;

		playing = true;
		timePlaying = 0.0f;
	}

	void ParticleSystem::SetMaterialInstance(MaterialInstance *mat)
	{
		matInstance = mat;
		materialUBOChanged = true;
	}

	void ParticleSystem::SetCenter(const glm::vec3 &center)
	{
		this->center = center;
	}

	void ParticleSystem::Serialize(Serializer &s)
	{
		s.Write(matInstance->path);
		s.Write(isLooping);
		s.Write(center);
		s.Write(duration);
		s.Write(startLifeTime);
		s.Write(emission);
		s.Write((int)emissionShape);

		if (emissionShape == EmissionShape::BOX)
			s.Write(emissionBox);
		else if (emissionShape == EmissionShape::SPHERE)
			s.Write(emissionRadius);

		s.Write(startSize);
		s.Write(useAtlas);
		s.Write(atlasInfo.nColumns);
		s.Write(atlasInfo.nRows);
		s.Write(atlasInfo.cycles);
		s.Write(startVelocity);
		s.Write(startColor);
		s.Write(maxParticles);
		s.Write(fadeAlphaOverLifetime);

		s.Write(useRandomVelocity);
		if (useRandomVelocity)
		{
			s.Write(velocityLow);
			s.Write(velocityHigh);
		}
	}

	void ParticleSystem::Deserialize(Serializer &s)
	{
		//std::string matPath;
		//s.Read(matPath);
		s.Read(isLooping);
		s.Read(center);
		s.Read(duration);
		s.Read(startLifeTime);
		s.Read(emission);
		s.Read(emissionShape);

		if (emissionShape == EmissionShape::BOX)
			s.Read(emissionBox);
		else if (emissionShape == EmissionShape::SPHERE)
			s.Read(emissionRadius);

		s.Read(startSize);
		s.Read(useAtlas);
		s.Read(atlasInfo.nColumns);
		s.Read(atlasInfo.nRows);
		s.Read(atlasInfo.cycles);
		s.Read(startVelocity);
		s.Read(startColor);
		s.Read(maxParticles);
		s.Read(fadeAlphaOverLifetime);

		s.Read(useRandomVelocity);
		if (useRandomVelocity)
		{
			s.Read(velocityLow);
			s.Read(velocityHigh);
		}
	}
}
