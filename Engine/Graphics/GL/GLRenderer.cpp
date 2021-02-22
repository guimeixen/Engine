#include "GLRenderer.h"

#include "Graphics/Shader.h"
#include "Program/Log.h"
#include "Graphics/Material.h"
#include "Graphics/Mesh.h"
#include "Graphics/Buffers.h"
#include "GLVertexArray.h"
#include "GLVertexBuffer.h"
#include "GLIndexBuffer.h"
#include "GLShader.h"
#include "GLDrawIndirectBuffer.h"
#include "GLSSBO.h"
#include "GLTexture2D.h"
#include "GLTexture3D.h"
#include "GLTextureCube.h"
#include "GLUniformBuffer.h"

#include "Program/Utils.h"
#include "Program/StringID.h"

#include "Data/Shaders/bindings.glsl"

#include "include/glm/gtc/matrix_transform.hpp"
#include "include/glm/gtc/type_ptr.hpp"

#include <iostream>
#include <iomanip>
#include <sstream>

namespace Engine
{
	GLRenderer::GLRenderer(FileManager *fileManager, GLuint width, GLuint height)
	{
		this->width = width;
		this->height = height;
		this->fileManager = fileManager;
		viewportPos = glm::vec2();
		currentMaterial = nullptr;
		materialUBO = nullptr;
		renderStats = {};

		currentAPI = GraphicsAPI::OpenGL;
	}

	GLRenderer::~GLRenderer()
	{
		Dispose();
	}

	bool GLRenderer::Init()
	{
		glewExperimental = GL_TRUE;

		GLenum err = glewInit();

		if (err != GLEW_OK)
		{
			Log::Print(LogLevel::LEVEL_ERROR, "Failed to initialize GLEW - Error: %d\n", err);
			return false;
		}
		else
			Log::Print(LogLevel::LEVEL_INFO, "GLEW successfuly initialized\n");

		//Log::Message("GL version: " + glGetString(GL_VERSION));
		//Log::Message("GPU: " << glGetString(GL_RENDERER));
		glEnable(GL_CULL_FACE);
		glCullFace(GL_BACK);
		glFrontFace(GL_CCW);
		glEnable(GL_DEPTH_TEST);
		glDepthFunc(GL_LESS);
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glGetError();									// GLEW produces an error
		glEnable(GL_BLEND);
		glEnable(GL_CLIP_DISTANCE0);

		if (GLEW_ARB_clip_control)
		{
			glClipControl(GL_LOWER_LEFT, GL_ZERO_TO_ONE);
		}
		else
		{
			return false;
		}
		if (!GLEW_ARB_base_instance)
		{
			std::cout << "Base instance not supported\n";
			return false;
		}

		unsigned int initialSize = 256000;		// bytes

		/*meshParamsUBO = new GLUniformBuffer(nullptr, initialSize);
		meshParamsUBO->BindTo(OBJECT_UBO_BINDING);
		meshParamsData.resize(2048 * 6);*/

		uboMinOffsetAlignment = 0;
		GLint alignment = 0;
		GLint size = 0;
		GLint m = 0;
		glGetIntegerv(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, &alignment);
		glGetIntegerv(GL_UNIFORM_BUFFER_START, &size);
		glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, &m);

		Log::Print(LogLevel::LEVEL_INFO, "max tex units:  %d\n", m);

		uboMinOffsetAlignment = (unsigned int)alignment;
		//std::cout << "Min UBO offset alignment: " << alignment << '\n';
		//std::cout << "Min block data size: " << size << '\n';

		// Instance buffer
		initialSize = 1024 * 512; // 512 kib
		glGenBuffers(1, &instanceDataSSBO);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, instanceDataSSBO);
		glBufferData(GL_SHADER_STORAGE_BUFFER, initialSize, nullptr, GL_DYNAMIC_COPY);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, INSTANCE_DATA_SSBO, instanceDataSSBO);

		instanceData.resize(2048 * 4);

		cameraUBO = new GLUniformBuffer(nullptr, sizeof(CameraUBO));
		cameraUBO->BindTo(CAMERA_UBO);

		materialUBO = new GLUniformBuffer(nullptr, 128);
		materialUBO->BindTo(MAT_PROPERTIES_UBO_BINDING);
		return true;
	}

	void GLRenderer::PostLoad()
	{
	}

	void GLRenderer::Resize(unsigned int width, unsigned int height)
	{
		this->width = width;
		this->height = height;

		camera->SetProjectionMatrix(camera->GetFov(), width, height, camera->GetNearPlane(), camera->GetFarPlane());
	}

	void GLRenderer::SetCamera(Camera *camera, const glm::vec4 &clipPlane)
	{
		this->camera = camera;

		CameraUBO ubo = {};
		ubo.proj = camera->GetProjectionMatrix();
		ubo.view = camera->GetViewMatrix();
		ubo.projView = ubo.proj * ubo.view;
		ubo.invView = glm::inverse(ubo.view);
		ubo.invProj = glm::inverse(ubo.proj);
		ubo.clipPlane = clipPlane;
		ubo.camPos = glm::vec4(camera->GetPosition(), 0.0f);
		ubo.nearFarPlane = glm::vec2(camera->GetNearPlane(), camera->GetFarPlane());
		
		cameraUBO->Update(&ubo, sizeof(ubo), 0);
	}

	void GLRenderer::UpdateUBO(Buffer* ubo, const void* data, unsigned int size, unsigned int offset)
	{
		ubo->Update(data, size, offset);
	}

	VertexArray *GLRenderer::CreateVertexArray(const VertexInputDesc &desc, Buffer *vertexBuffer, Buffer *indexBuffer)
	{
		return new GLVertexArray(desc, vertexBuffer, indexBuffer);
	}

	VertexArray *GLRenderer::CreateVertexArray(const VertexInputDesc *descs, unsigned int descCount, const std::vector<Buffer*> &vertexBuffers, Buffer *indexBuffer)
	{
		return new GLVertexArray(descs, descCount, vertexBuffers, indexBuffer);
	}

	Buffer *GLRenderer::CreateVertexBuffer(const void *data, unsigned int size, BufferUsage usage)
	{
		return new GLVertexBuffer(data, size, usage);
	}

	Buffer *GLRenderer::CreateIndexBuffer(const void *data, unsigned int size, BufferUsage usage)
	{
		return new GLIndexBuffer(data, size, usage);
	}

	Buffer *GLRenderer::CreateUniformBuffer(const void *data, unsigned int size)
	{
		return new GLUniformBuffer(data, size);
	}

	Buffer *GLRenderer::CreateDrawIndirectBuffer(unsigned int size, const void *data)
	{
		return new GLDrawIndirectBuffer(data, size);
	}

	Buffer *GLRenderer::CreateSSBO(unsigned int size, const void *data, unsigned int stride, BufferUsage usage)
	{
		return new GLSSBO(data, size);
	}

	Framebuffer *GLRenderer::CreateFramebuffer(const FramebufferDesc &desc)
	{
		Framebuffer *fb = new GLFramebuffer(desc);
		//fb->AddReAddReference();
		return fb;
	}

	ShaderProgram* GLRenderer::CreateShader(const std::string &vertexName, const std::string &fragmentName, const std::string &defines, const std::vector<VertexInputDesc> &descs, const BlendState &blendState)
	{
		unsigned int id = SID(vertexName + fragmentName + defines);

		// If the shader already exists return that one instead
		if (shaderPrograms.find(id) != shaderPrograms.end())
		{
			return shaderPrograms[id];
		}

		// Else create a new one
		ShaderProgram* shader = new GLShader(defines, vertexName, fragmentName);
		shaderPrograms[id] = shader;

		return shader;
	}

	ShaderProgram* GLRenderer::CreateShader(const std::string &vertexName, const std::string &fragmentName, const std::vector<VertexInputDesc> &descs, const BlendState &blendState)
	{
		return CreateShader(vertexName, fragmentName, "", descs, blendState);
	}

	ShaderProgram* GLRenderer::CreateShaderWithGeometry(const std::string &vertexPath, const std::string &geometryPath, const std::string &fragmentPath, const std::string &defines, const std::vector<VertexInputDesc> &descs)
	{
		unsigned int id = SID(vertexPath + geometryPath + fragmentPath + defines);

		// If the shader already exists return that one instead
		if (shaderPrograms.find(id) != shaderPrograms.end())
		{
			return shaderPrograms[id];
		}

		// Else create a new one
		ShaderProgram* shader = new GLShader(defines, vertexPath, geometryPath, fragmentPath);
		shaderPrograms[id] = shader;

		return shader;
	}

	ShaderProgram* GLRenderer::CreateShaderWithGeometry(const std::string &vertexPath, const std::string &geometryPath, const std::string &fragmentPath, const std::vector<VertexInputDesc> &descs)
	{
		return CreateShaderWithGeometry(vertexPath, geometryPath, fragmentPath, "", descs);
	}

	ShaderProgram* GLRenderer::CreateComputeShader(const std::string &computePath, const std::string &defines)
	{
		unsigned int id = SID(computePath + defines);

		// If the shader already exists return that one instead
		if (shaderPrograms.find(id) != shaderPrograms.end())
		{
			return shaderPrograms[id];
		}

		// Else create a new one
		ShaderProgram* shader = new GLShader(defines, computePath);
		shaderPrograms[id] = shader;

		return shader;
	}

	ShaderProgram* GLRenderer::CreateComputeShader(const std::string &computePath)
	{
		return CreateComputeShader(computePath, "");
	}

	MaterialInstance *GLRenderer::CreateMaterialInstance(ScriptManager &scriptManager, const std::string &matInstPath, const std::vector<VertexInputDesc>& inputDescs)
	{
		MaterialInstance *m = Material::LoadMaterialInstance(this, matInstPath, scriptManager, inputDescs);
		materialInstances.push_back(m);

		return m;		// We need unique instances. eg If one button would get pressed and change the texture, it would also change for every other button using the same mat.instance
	}

	MaterialInstance *GLRenderer::CreateMaterialInstanceFromBaseMat(ScriptManager &scriptManager, const std::string &baseMatPath, const std::vector<VertexInputDesc> &inputDescs)
	{
		MaterialInstance *m = Material::LoadMaterialInstanceFromBaseMat(this, baseMatPath, scriptManager, inputDescs);
		materialInstances.push_back(m);

		return m;
	}

	Texture *GLRenderer::CreateTexture2D(const std::string &path, const TextureParams &params, bool storeTextureData)
	{
		unsigned int id = SID(path);

		// If the texture already exists return that one instead
		if (textures.find(id) != textures.end())
		{
			Texture *tex = textures[id];
			tex->AddReference();
			return textures[id];
		}

		Texture *tex = new GLTexture2D(path, params, storeTextureData);
		tex->AddReference();
		textures[id] = tex;

		return tex;
	}

	Texture *GLRenderer::CreateTexture3D(const std::string &path, const void *data, unsigned int width, unsigned int height, unsigned int depth, const TextureParams &params)
	{
		unsigned int id = SID(path);

		// If the texture already exists return that one instead
		if (textures.find(id) != textures.end())
		{
			Texture *tex = textures[id];
			tex->AddReference();
			return textures[id];
		}

		Texture *tex = new GLTexture3D(width, height, depth, params, nullptr);
		tex->AddReference();
		textures[id] = tex;

		return tex;
	}

	Texture *GLRenderer::CreateTextureCube(const std::vector<std::string> &faces, const TextureParams &params)
	{
		unsigned int id = SID(faces[0]);			// Better way? Otherwise if faces[0] is equal to an already stored texture but the other faces are different it won't get loaded

		// If the texture already exists return that one instead
		if (textures.find(id) != textures.end())
		{
			Texture *tex = textures[id];
			tex->AddReference();
			return textures[id];
		}

		Texture *tex = new GLTextureCube(faces, params);
		tex->AddReference();
		textures[id] = tex;

		return tex;
	}

	Texture *GLRenderer::CreateTextureCube(const std::string &path, const TextureParams &params)
	{
		unsigned int id = SID(path);

		// If the texture already exists return that one instead
		if (textures.find(id) != textures.end())
		{
			Texture *tex = textures[id];
			tex->AddReference();
			return textures[id];
		}

		Texture *tex = new GLTextureCube(path, params);
		tex->AddReference();
		textures[id] = tex;

		return tex;
	}

	Texture *GLRenderer::CreateTexture2DFromData(unsigned int width, unsigned int height, const TextureParams &params, const void *data)
	{
		return new GLTexture2D(width, height, params, data);
	}

	Texture *GLRenderer::CreateTexture3DFromData(unsigned int width, unsigned int height, unsigned int depth, const TextureParams &params, const void *data)
	{
		return new GLTexture3D(width, height, depth, params, data);
	}

	void GLRenderer::SetRenderTarget(Framebuffer *rt)
	{
		rt->Bind();
	}

	void GLRenderer::SetRenderTargetAndClear(Framebuffer *rt)
	{
		if (rt->AreWritesDisabled() == false)
		{
			rt->Bind();
			if (depthStencilState.depthWrite == false)			// Depth writing must be enabled for the depth buffer to be cleared
				glDepthMask(GL_TRUE);
			rt->Clear();
		}
		else
		{
			glBindFramebuffer(GL_FRAMEBUFFER, 0);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		}

		glViewport(0, 0, rt->GetWidth(), rt->GetHeight());
	}

	void GLRenderer::ClearRenderTarget(Framebuffer *rt)
	{
		if (depthStencilState.depthWrite == false)
			glDepthMask(GL_TRUE);
		rt->Clear();
	}

	void GLRenderer::SetViewport(const Viewport &viewport)
	{
		glViewport(viewport.x, viewport.y, viewport.width, viewport.height);
	}

	void GLRenderer::SetDefaultRenderTarget()
	{
		if (depthStencilState.depthWrite == false)			// Depth writing must be enabled for the depth buffer to be cleared
			glDepthMask(GL_TRUE);
		GLFramebuffer::SetDefault(width, height);
	}

	void GLRenderer::Submit(const RenderQueue &renderQueue)
	{
		//meshParamsOffset = 0;
		instanceDataOffset = 0;
		for (size_t i = 0; i < renderQueue.size(); i++)
		{
			const RenderItem &ri = renderQueue[i];

			/*if (ri.meshParams != nullptr)
			{
				if (meshParamsOffset > meshParamsData.size())
					meshParamsData.resize(meshParamsData.size() * 2);

				std::memcpy(buffer + meshParamsOffset / 4, ri.meshParams, ri.meshParamsSize);			// Size must be in bytes
				meshParamsOffset += ri.meshParamsSize;		// Offset must be in elements
				meshParamsOffset += (uboMinOffsetAlignment - (meshParamsOffset % uboMinOffsetAlignment));
			}*/

			if (ri.instanceData)
			{
				if (instanceDataOffset > instanceData.size())
					instanceData.resize(instanceData.size() * 2);

				memcpy(instanceBuffer + instanceDataOffset / 4, ri.instanceData, ri.instanceDataSize);
				instanceDataOffset += ri.instanceDataSize;
			}
			if (ri.meshParams)
			{
				// handle resize
				memcpy(instanceBuffer + instanceDataOffset / 4, ri.meshParams, ri.meshParamsSize);
				instanceDataOffset += ri.meshParamsSize;
			}
		}

		// TODO: Check if the data won't fit in the buffer and resize it
		//if (meshParamsOffset > 0)
		//	meshParamsUBO->Update(buffer, meshParamsOffset * 4, 0);			// Size must be in bytes

		if (instanceDataOffset > 0)
		{
			glBindBuffer(GL_SHADER_STORAGE_BUFFER, instanceDataSSBO);
			glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, instanceDataOffset, instanceBuffer);
			//glNamedBufferSubData(instanceDataSSBO, 0, instanceDataOffset, instanceBuffer);
		}

		//meshParamsOffset = 0;
		instanceDataOffset = 0;

		for (size_t i = 0; i < renderQueue.size(); i++)
		{
			Submit(renderQueue[i]);
		}
		//meshParamsOffset = 0;
		instanceDataOffset = 0;
	}

	void GLRenderer::Submit(const RenderItem &renderItem)
	{
		// TODO: Sort by material and use one big ubo to store material data and then use glBindBufferRange for each material change to access it's data
		if (renderItem.materialData)
			materialUBO->Update(renderItem.materialData, renderItem.materialDataSize, 0);		// TODO: Check what's faster - Update the UBO for every ri or use one BIG UBO and use glBindBufferRange()

		/*if (renderItem.meshParams != nullptr)
		{
			// Offset must be a multiple of GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT
			glBindBufferRange(GL_UNIFORM_BUFFER, OBJECT_UBO_BINDING, static_cast<GLUniformBuffer*>(meshParamsUBO)->GetID(), meshParamsOffset, renderItem.meshParamsSize);		// Everything in bytes
			meshParamsOffset += renderItem.meshParamsSize;		// bytes
			meshParamsOffset += (uboMinOffsetAlignment - (meshParamsOffset % uboMinOffsetAlignment));
		}*/

		const ShaderPass &pass = renderItem.matInstance->baseMaterial->GetShaderPass(renderItem.shaderPass);

		GLShader *s = static_cast<GLShader*>(pass.shader);

		SetShader(s->GetProgram());
		if (renderItem.transform)
			s->SetModelMatrix(*renderItem.transform);

		if (renderItem.meshParams)
		{
			s->SetStartIndex(instanceDataOffset);
			instanceDataOffset += renderItem.meshParamsSize;
		}

		if (renderItem.instanceData)
		{
			s->SetInstanceDataOffset((int)instanceDataOffset / sizeof(glm::mat4));
			instanceDataOffset += renderItem.instanceDataSize;
		}
		else
		{
			s->SetInstanceDataOffset(-1);
		}

		for (size_t i = 0; i < renderItem.matInstance->textures.size(); i++)
		{
			Texture *t = renderItem.matInstance->textures[i];
			
			if (t)
			{
				const TextureParams &p = t->GetTextureParams();
				if (!p.usedAsStorageInGraphics)
				{
					t->Bind(FIRST_TEXTURE + i);
					//t->Bind(i + currentTextureBinding);			// currenTextureBinding acts as FIRST_TEXTURE_SLOT
					renderStats.textureChanges++;
				}
				//SetTexture(t->GetID(), i);
			}
		}

		SetBlendState(pass.blendState);
		SetDepthStencilState(pass.depthStencilState);
		SetRasterizerState(pass.rasterizerState);

		glBindVertexArray(static_cast<GLVertexArray*>(renderItem.mesh->vao)->GetID());

		if (renderItem.mesh->instanceCount > 0)
		{
			renderStats.instanceCount += renderItem.mesh->instanceCount;
			if (renderItem.mesh->vertexCount > 0)
				glDrawArraysInstanced(pass.topology, 0, renderItem.mesh->vertexCount, renderItem.mesh->instanceCount);
			else
				glDrawElementsInstancedBaseInstance(pass.topology, renderItem.mesh->indexCount, GL_UNSIGNED_SHORT, (void*)renderItem.mesh->indexOffset, renderItem.mesh->instanceCount, renderItem.mesh->instanceOffset);
		}
		else
		{
			if (renderItem.mesh->vertexCount > 0)
				glDrawArrays(pass.topology, 0, renderItem.mesh->vertexCount);
			else
				glDrawElementsBaseVertex(pass.topology, renderItem.mesh->indexCount, GL_UNSIGNED_SHORT, (void*)renderItem.mesh->indexOffset, renderItem.mesh->vertexOffset);
		}
		renderStats.drawCalls++;
		renderStats.triangles += renderItem.mesh->indexCount + renderItem.mesh->vertexCount;
	}

	void GLRenderer::SubmitIndirect(const RenderItem &renderItem, Buffer *indirectBuffer)
	{
		if (renderItem.materialData)
			materialUBO->Update(renderItem.materialData, renderItem.materialDataSize, 0);

		/*if (renderItem.meshParams != nullptr)
		{
			// Offset must be a multiple of GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT
			glBindBufferRange(GL_UNIFORM_BUFFER, OBJECT_UBO_BINDING, static_cast<GLUniformBuffer*>(meshParamsUBO)->GetID(), meshParamsOffset, renderItem.meshParamsSize);		// Everything in bytes
			meshParamsOffset += renderItem.meshParamsSize;		// bytes
			meshParamsOffset += (uboMinOffsetAlignment - (meshParamsOffset % uboMinOffsetAlignment));
		}*/

		const ShaderPass &pass = renderItem.matInstance->baseMaterial->GetShaderPass(renderItem.shaderPass);

		GLShader *s = static_cast<GLShader*>(pass.shader);

		SetShader(s->GetProgram());

		glBindBuffer(GL_DRAW_INDIRECT_BUFFER, static_cast<GLDrawIndirectBuffer*>(indirectBuffer)->GetID());

		/*if (renderItem.instanceData != nullptr)
		{
			s->SetInstanceDataOffset((int)instanceDataOffset / sizeof(glm::mat4));
			instanceDataOffset += renderItem.instanceDataSize;
		}
		else
		{
			s->SetInstanceDataOffset(-1);
		}*/

		for (size_t i = 0; i < renderItem.matInstance->textures.size(); i++)
		{
			Texture *t = renderItem.matInstance->textures[i];

			if (t)
			{
				/*const TextureParams &p = t->GetTextureParams();
				if (p.usedAsStorageInGraphics)
				{

				}
				else
				{

				}*/
				t->Bind(FIRST_TEXTURE + i);
				renderStats.textureChanges++;
				//SetTexture(t->GetID(), i);
			}
		}

		SetBlendState(pass.blendState);
		SetDepthStencilState(pass.depthStencilState);
		SetRasterizerState(pass.rasterizerState);

		glBindVertexArray(static_cast<GLVertexArray*>(renderItem.mesh->vao)->GetID());

		if (renderItem.mesh->vertexCount > 0)
			glDrawArraysIndirect(GL_TRIANGLES, nullptr);
		else
			glDrawElementsIndirect(GL_TRIANGLES, GL_UNSIGNED_SHORT, nullptr);

		/*if (renderItem.mesh->instanceCount > 0)
		{
			renderStats.instanceCount += renderItem.mesh->instanceCount;
			if (renderItem.mesh->vertexCount > 0)
				glDrawArraysInstanced(pass.topology, 0, renderItem.mesh->vertexCount, renderItem.mesh->instanceCount);
			else
				glDrawElementsInstancedBaseInstance(pass.topology, renderItem.mesh->indexCount, GL_UNSIGNED_SHORT, (void*)renderItem.mesh->indexOffset, renderItem.mesh->instanceCount, renderItem.mesh->instanceOffset);
		}
		else
		{
			if (renderItem.mesh->vertexCount > 0)
				glDrawArrays(pass.topology, 0, renderItem.mesh->vertexCount);
			else
				glDrawElementsBaseVertex(pass.topology, renderItem.mesh->indexCount, GL_UNSIGNED_SHORT, (void*)renderItem.mesh->indexOffset, renderItem.mesh->vertexOffset);
		}*/
		renderStats.drawCalls++;
	}

	void GLRenderer::Dispatch(const DispatchItem &item)
	{
		/*if (data)
		{
			meshParamsUBO->Update(data, size, 0);
			glBindBufferRange(GL_UNIFORM_BUFFER, OBJECT_UBO_BINDING, static_cast<GLUniformBuffer*>(meshParamsUBO)->GetID(), 0, size);
		}*/

		if (item.materialData)
			materialUBO->Update(item.materialData, item.materialDataSize, 0);

		const ShaderPass &pass = item.matInstance->baseMaterial->GetShaderPass(item.shaderPass);

		GLShader *s = static_cast<GLShader*>(pass.shader);
		SetShader(s->GetProgram());

		for (size_t i = 0; i < item.matInstance->textures.size(); i++)
		{
			Texture *t = item.matInstance->textures[i];

			if (t)
			{
				const TextureParams &p = t->GetTextureParams();
				/*if (p.usedAsStorageInCompute)
				{
					//t->BindAsImage(i, 0, false, ImageAccess::WRITE_ONLY, TextureInternalFormat::RGBA8);
					t->BindAsImage(i, 0, false, ImageAccess::WRITE_ONLY, p.internalFormat);
				}
				else
				{
					t->Bind(i);
				}*/

				if (!p.usedAsStorageInCompute)
				{
					//t->Bind(i + currentTextureBinding);
					t->Bind(FIRST_TEXTURE + i);
				}
				
				renderStats.textureChanges++;
				//SetTexture(t->GetID(), i);
			}
		}

		/*for (size_t i = 0; i < item.matInstance->buffers.size(); i++)
		{
			Buffer *buf = item.matInstance->buffers[i];
			if (buf->GetType() == BufferType::ShaderStorageBuffer)
			{
				GLSSBO *ssbo = static_cast<GLSSBO*>(buf);
				ssbo->BindTo(i + 6);
			}
			else if (buf->GetType() == BufferType::DrawIndirectBuffer)
			{

			}
		}*/

		glDispatchCompute(item.numGroupsX, item.numGroupsY, item.numGroupsZ);
		renderStats.dispatchCalls++;
	}

	void GLRenderer::AddTextureResourceToSlot(unsigned int binding, Texture *texture, bool useStorage, unsigned int stages, bool separateMipViews)
	{
		// Even though texture and images bindings are separate in OpenGL, we treat it as if they use the same
		// bindings points to help with the Vulkan implementation. I don't think there are performance issues
		// by leaving gaps in the bindings? OpenGL maybe collapses unused ones?

		if (!texture)
			return;

		if (texture->GetType() == TextureType::TEXTURE2D)
		{
			const TextureParams &p = texture->GetTextureParams();

			if ((p.usedAsStorageInCompute || p.usedAsStorageInGraphics) && useStorage)
			{
				GLTexture2D *tex = static_cast<GLTexture2D*>(texture);
				//tex->BindAsImage(binding, 0, false, ImageAccess::READ_ONLY);
			}
			else
			{
				GLTexture2D *tex = static_cast<GLTexture2D*>(texture);
				tex->Bind(binding);
			}
		}
		else if (texture->GetType() == TextureType::TEXTURE3D)
		{
			const TextureParams &p = texture->GetTextureParams();

			if ((p.usedAsStorageInCompute || p.usedAsStorageInGraphics) && useStorage)
			{
				GLTexture3D *tex = static_cast<GLTexture3D*>(texture);
				//tex->BindAsImage(currentTextureBinding);
			}
			else
			{
				GLTexture3D *tex = static_cast<GLTexture3D*>(texture);
				tex->Bind(binding);			
			}
		}

		currentTextureBinding++;
	}

	void GLRenderer::AddBufferResourceToSlot(unsigned int binding, Buffer *buffer, unsigned int stages)
	{
		if (!buffer)
			return;

		if (buffer->GetType() == BufferType::UniformBuffer)
		{
			GLUniformBuffer *ubo = static_cast<GLUniformBuffer*>(buffer);
			ubo->BindTo(binding);
		}
		else if (buffer->GetType() == BufferType::ShaderStorageBuffer)
		{
			GLSSBO *ssbo = static_cast<GLSSBO*>(buffer);
			ssbo->BindTo(binding);
		}
		else if (buffer->GetType() == BufferType::DrawIndirectBuffer)
		{
			GLDrawIndirectBuffer *indBuffer = static_cast<GLDrawIndirectBuffer*>(buffer);
			indBuffer->BindTo(binding);
		}

		//currentBinding++;
	}

	void GLRenderer::SetupResources()
	{
	}

	void GLRenderer::UpdateTextureResourceOnSlot(unsigned int binding, Texture *texture, bool useStorage, bool separateMipViews)
	{
	}

	void GLRenderer::PerformBarrier(const Barrier &barrier)
	{
		GLbitfield barrierBits = 0;

		for (size_t i = 0; i < barrier.images.size(); i++)
		{
			// Only perform the barrier when we go from write to read
			if (barrier.images[i].readToWrite == false)
			{
				barrierBits |= GL_TEXTURE_FETCH_BARRIER_BIT | GL_SHADER_IMAGE_ACCESS_BARRIER_BIT;
				break;
			}
		}

		for (size_t i = 0; i < barrier.buffers.size(); i++)
		{
			const BarrierBuffer &bb = barrier.buffers[i];

			// Only perform the barrier when we go from write to read
			if (bb.readToWrite == false)
			{
				if (bb.buffer->GetType() == BufferType::DrawIndirectBuffer)
					barrierBits |= GL_COMMAND_BARRIER_BIT;
				else if (bb.buffer->GetType() == BufferType::ShaderStorageBuffer)
					barrierBits |= GL_SHADER_STORAGE_BARRIER_BIT;
			}
		}

		if (barrierBits != 0)
			glMemoryBarrier(barrierBits);
	}

	void GLRenderer::BindImage(unsigned int slot, unsigned int mipLevel, Texture *tex, ImageAccess access)
	{
		if(tex->GetType()==TextureType::TEXTURE2D)
			tex->BindAsImage(slot, mipLevel, false, access, tex->GetTextureParams().internalFormat);
		else
			tex->BindAsImage(slot, mipLevel, true, access, tex->GetTextureParams().internalFormat);
	}

	void GLRenderer::CopyImage(Texture *src, Texture *dst)
	{
		GLTexture2D *srcTex = static_cast<GLTexture2D*>(src);
		GLTexture2D *dstTex = static_cast<GLTexture2D*>(dst);
		glCopyImageSubData(srcTex->GetID(), GL_TEXTURE_2D, 0, 0, 0, 0, dstTex->GetID(), GL_TEXTURE_2D, 0, 0, 0, 0, srcTex->GetWidth(), srcTex->GetHeight(), 1);
	}

	void GLRenderer::ClearImage(Texture *tex)
	{
		if (tex->GetType() == TextureType::TEXTURE3D)
		{
			GLTexture3D *t = static_cast<GLTexture3D*>(tex);

			unsigned char clearColor[4] = { 0,0,0,0 };
			glClearTexImage(t->GetID(), 0, GL_RGBA, GL_UNSIGNED_BYTE, clearColor);
		}		
	}

	/*void GLRenderer::SetWireframe(bool enable)
	{
		if (enable == false)
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		else
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	}*/

	void GLRenderer::UpdateMaterialInstance(MaterialInstance *matInst)
	{
	}

	void GLRenderer::ReloadShaders()
	{
		for (size_t i = 0; i < materialInstances.size(); i++)
		{
			MaterialInstance* mi = materialInstances[i];

			std::vector<ShaderPass>& passes = mi->baseMaterial->GetShaderPasses();

			for (size_t i = 0; i < passes.size(); i++)
			{
				ShaderPass& sp = passes[i];

				sp.shader->CheckIfModifiedAndReload();
			}
		}
	}

	void GLRenderer::BeginFrame()
	{
		renderStats = {};
	}

	void GLRenderer::Dispose()
	{
		for (auto it = shaderPrograms.begin(); it != shaderPrograms.end(); it++)
		{
			delete it->second;
		}
		for (auto it = textures.begin(); it != textures.end(); it++)
		{
			it->second->RemoveReference();
		}
		for (size_t i = 0; i < materialInstances.size(); i++)
		{
			delete materialInstances[i];
			materialInstances[i] = nullptr;
		}
		materialInstances.clear();

		if (cameraUBO)
			delete cameraUBO;
		if (materialUBO)
			delete materialUBO;

		cameraUBO = nullptr;
		materialUBO = nullptr;

		/*if (meshParamsUBO)
		{
			delete meshParamsUBO;
			meshParamsUBO = nullptr;
		}*/

		Log::Print(LogLevel::LEVEL_INFO, "GL renderer exiting\n");
	}

	void GLRenderer::SortCommands()
	{
		// Sort the render commands by VAO, shader, texture
		/*std::sort(renderCmds.begin(), renderCmds.end(),
			[this](const RenderCommand &a, const RenderCommand &b) -> bool
		{
			/*if (a.mesh->vertexType != b.mesh->vertexType)
			{
				return dataManager->GetVAO(a.mesh->vertexType) < dataManager->GetVAO(b.mesh->vertexType);
			}
			return a.mat->GetShader()->GetProgram() < b.mat->GetShader()->GetProgram();
		});*/
	}

	void GLRenderer::SetBlendState(const BlendState &state)
	{
		bool changed = false;

		if (blendState.enableBlending != state.enableBlending)
		{
			changed = true;
			if (state.enableBlending)
				glEnable(GL_BLEND);
			else
				glDisable(GL_BLEND);
		}

		if (blendState.srcAlphaFactor != state.srcAlphaFactor ||
			blendState.dstAlphaFactor != state.dstAlphaFactor ||
			blendState.srcColorFactor != state.srcColorFactor ||
			blendState.dstColorFactor != state.dstColorFactor)
		{
			changed = true;
			glBlendFuncSeparate(state.srcColorFactor, state.dstColorFactor, state.srcAlphaFactor, state.dstAlphaFactor);
		}

		if (blendState.enableColorWriting != state.enableColorWriting)
		{
			changed = true;
			if (state.enableColorWriting)
				glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
			else
				glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
		}

		if (changed)
			blendState = state;
	}

	void GLRenderer::SetDepthStencilState(const DepthStencilState &state)
	{
		bool changed = false;			// Using this variable so if depth enable is different we don't updated the state there because the depth write if would not pass because they would be equal
		if (depthStencilState.depthEnable != state.depthEnable)
		{
			changed = true;
			if (state.depthEnable)
				glEnable(GL_DEPTH_TEST);
			else
				glDisable(GL_DEPTH_TEST);
		}
		if (depthStencilState.depthWrite != state.depthWrite)
		{
			changed = true;
			if (state.depthWrite)
				glDepthMask(GL_TRUE);
			else
				glDepthMask(GL_FALSE);
		}
		if (depthStencilState.depthFunc != state.depthFunc)
		{
			changed = true;
			glDepthFunc(state.depthFunc);
		}

		if (changed)
			depthStencilState = state;
	}

	void GLRenderer::SetRasterizerState(const RasterizerState &state)
	{
		bool changed = false;
		if (rasterizerState.cullFace != state.cullFace)
		{
			changed = true;
			glCullFace(state.cullFace);
		}
		if (rasterizerState.enableCulling != state.enableCulling)
		{
			changed = true;
			if (state.enableCulling)
				glEnable(GL_CULL_FACE);
			else
				glDisable(GL_CULL_FACE);
		}

		if (changed)
		{
			rasterizerState = state;
			renderStats.cullingChanges++;
		}
	}

	void GLRenderer::SetShader(unsigned int shader)
	{
		if (currentShader != shader)
		{
			currentShader = shader;
			glUseProgram(shader);
			renderStats.shaderChanges++;
		}
	}

	void GLRenderer::SetTexture(unsigned int id, unsigned int slot)
	{
		if (currentTextures[slot] != id)
		{
			glBindTextureUnit(slot, id);
			currentTextures[slot] = id;
			renderStats.textureChanges++;
		}
	}
}
