#include "D3D11Renderer.h"

#include "D3D11VertexBuffer.h"
#include "D3D11IndexBuffer.h"
#include "D3D11VertexArray.h"
#include "D3D11Shader.h"
#include "D3D11Texture2D.h"
#include "D3D11Texture3D.h"
#include "D3D11Framebuffer.h"
#include "D3D11SSBO.h"
#include "D3D11DrawIndirectBuffer.h"

#include "Graphics/Material.h"

#include "Program/StringID.h"

#include "include/glm/gtc/matrix_transform.hpp"
#include "include/glm/gtx/euler_angles.hpp"
#include <stdexcept>
#include <iostream>

namespace Engine
{
	D3D11Renderer::D3D11Renderer(HWND hwnd, unsigned int width, unsigned int height)
	{
		this->hwnd = hwnd;
		this->width = width;
		this->height = height;
		currentAPI = GraphicsAPI::D3D11;

		clearColor[0] = 0.0f;
		clearColor[1] = 0.0f;
		clearColor[2] = 0.0f;
		clearColor[3] = 1.0f;

		cameraUBO = nullptr;
		//frameUBO = nullptr;
		materialDataUBO = nullptr;
		instanceDataSSBO = nullptr;
		dispatchParamsUBO = nullptr;

		renderTargetView = nullptr;
		depthStencilTexture = nullptr;
		depthStencilView = nullptr;

		currentStateID = std::numeric_limits<unsigned int>::max();
	}

	D3D11Renderer::~D3D11Renderer()
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
			if (materialInstances[i])
			{
				delete materialInstances[i];
				materialInstances[i] = nullptr;
			}
		}
		for (size_t i = 0; i < ubos.size(); i++)
		{
			if (ubos[i])
				delete ubos[i];
		}
		for (size_t i = 0; i < ssbos.size(); i++)
		{
			if (ssbos[i])
				ssbos[i]->RemoveReference();
		}

		/*if (rsState)
			rsState->Release();
		if (dsState)
			dsState->Release();
		if (blendState)
			blendState->Release();
		*/
		for (size_t i = 0; i < states.size(); i++)
		{
			const State &s = states[i];
			if (s.rsState)
				s.rsState->Release();
			if (s.dsState)
				s.dsState->Release();
			if (s.blendState)
				s.blendState->Release();
		}

		if (cameraUBO)
			delete cameraUBO;
		//if (frameUBO)
		//	delete frameUBO;
		if (materialDataUBO)
			delete materialDataUBO;
		if (instanceDataSSBO)
			delete instanceDataSSBO;
		if (dispatchParamsUBO)
			delete dispatchParamsUBO;

		if (renderTargetView)
			renderTargetView->Release();
		if (depthStencilTexture)
			depthStencilTexture->Release();
		if (depthStencilView)
			depthStencilView->Release();
		if (swapChain)
			swapChain->Release();
		if (immediateContext)
			immediateContext->Release();

		//ID3D11Debug *debug = nullptr;
		//device->QueryInterface(__uuidof(ID3D11Debug), reinterpret_cast<void**>(&debug));
		
		if (device)
			device->Release();
		
		//debug->ReportLiveDeviceObjects(D3D11_RLDO_DETAIL);
		//std::cout << "D3D11 Renderer shutdown\n";
	}

	bool D3D11Renderer::Init()
	{
		unsigned int createDeviceFlags = 0;

#ifdef _DEBUG
		createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

		D3D_FEATURE_LEVEL featureLevel;

		// Get feature level
		HRESULT hr = D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0, nullptr, 0, D3D11_SDK_VERSION, nullptr, &featureLevel, nullptr);

		if (FAILED(hr))
		{
			throw std::runtime_error("Failed to get feature level");
			return false;
		}

		// Create device and swap chain
		DXGI_SWAP_CHAIN_DESC desc = {};
		desc.BufferCount = 1;
		desc.BufferDesc.Width = width;
		desc.BufferDesc.Height = height;
		desc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		desc.BufferDesc.RefreshRate.Numerator = 60;
		desc.BufferDesc.RefreshRate.Denominator = 1;
		desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		desc.OutputWindow = hwnd;
		desc.SampleDesc.Count = 1;
		desc.SampleDesc.Quality = 0;
		desc.Windowed = TRUE;

		hr = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, createDeviceFlags, &featureLevel, 1, D3D11_SDK_VERSION, &desc, &swapChain, &device, nullptr, &immediateContext);

		if (FAILED(hr))
		{
			throw std::runtime_error("Failed to get create device and swap chain");
			return false;
		}

		CreateBackBufferRTVAndDSV();

		cameraUBO = new D3D11UniformBuffer(device, immediateContext, nullptr, sizeof(CameraUBO), BufferUsage::DYNAMIC);
		materialDataUBO = new D3D11UniformBuffer(device, immediateContext, nullptr, 128, BufferUsage::DYNAMIC);
		dispatchParamsUBO = new D3D11UniformBuffer(device, immediateContext, nullptr, sizeof(DispatchParams), BufferUsage::DYNAMIC);		// uvec3 for numWorkGroups.xyz and another for padding

		instanceDataSSBO = new D3D11SSBO(device, immediateContext, nullptr, 1024 * 256, sizeof(glm::mat4), BufferUsage::DYNAMIC);		// 256 kib

		ID3D11Buffer *camCB = cameraUBO->GetBuffer();
		ID3D11Buffer *matCB = materialDataUBO->GetBuffer();
		ID3D11Buffer *dispatchCB = dispatchParamsUBO->GetBuffer();
		
		ID3D11ShaderResourceView *instDataSRV = instanceDataSSBO->GetSRV();
		
		ID3D11Buffer *cbs[] = { camCB, matCB, dispatchCB };

		immediateContext->VSSetConstantBuffers(0, 3, cbs);
		immediateContext->GSSetConstantBuffers(0, 3, cbs);
		immediateContext->PSSetConstantBuffers(0, 3, cbs);
		immediateContext->CSSetConstantBuffers(0, 3, cbs);

		immediateContext->VSSetShaderResources(0, 1, &instDataSRV);

		currentCBBinding = 3;
		currentSRVBinding = 1;
		
		return true;
	}

	void D3D11Renderer::PostLoad()
	{
		/*for (size_t i = 0; i < ubos.size(); i++)
		{
			ID3D11Buffer *ubo = ubos[i]->GetBuffer();
			immediateContext->PSSetConstantBuffers(cbufferOffset, 1, &ubo);
			immediateContext->VSSetConstantBuffers(cbufferOffset, 1, &ubo);

			cbufferOffset++;
		}

		srvOffset += 1;		// For the csm texture*/
	}

	void D3D11Renderer::Resize(unsigned int width, unsigned int height)
	{
		this->width = width;
		this->height = height;

		CreateBackBufferRTVAndDSV();
	}

	void D3D11Renderer::SetCamera(Camera *camera, const glm::vec4 &clipPlane)
	{
		glm::mat4 proj = camera->GetProjectionMatrix();
		glm::mat4 view = camera->GetViewMatrix();
		
		CameraUBO ubo = {};
		ubo.proj = glm::transpose(proj);
		ubo.view = glm::transpose(view);
		ubo.projView = glm::transpose(proj * view);
		ubo.invProj = glm::inverse(ubo.proj);		// Check if transpose is needed: transpose(inverse(proj)) instead of inverse(ubo.proj)
		ubo.invView = glm::inverse(ubo.view);
		ubo.clipPlane = clipPlane;
		ubo.camPos = glm::vec4(camera->GetPosition(), 0.0f);
		ubo.nearFarPlane = glm::vec2(camera->GetNearPlane(), camera->GetFarPlane());

		cameraUBO->Update(&ubo, sizeof(CameraUBO), 0);
	}

	void D3D11Renderer::UpdateBuffer(Buffer* ubo, const void* data, unsigned int size, unsigned int offset)
	{
		ubo->Update(data, size, offset);
	}

	void D3D11Renderer::BeginFrame()
	{
	}

	void D3D11Renderer::Present()
	{
		swapChain->Present(0, 0);
	}

	VertexArray *D3D11Renderer::CreateVertexArray(const VertexInputDesc &desc, Buffer *vertexBuffer, Buffer *indexBuffer)
	{
		return new D3D11VertexArray(device, desc, vertexBuffer, indexBuffer);
	}

	VertexArray *D3D11Renderer::CreateVertexArray(const VertexInputDesc *descs, unsigned int descCount, const std::vector<Buffer*>& vertexBuffers, Buffer *indexBuffer)
	{
		return new D3D11VertexArray(device, descs, descCount, vertexBuffers, indexBuffer);
	}

	Buffer *D3D11Renderer::CreateVertexBuffer(const void *data, unsigned int size, BufferUsage usage)
	{
		return new D3D11VertexBuffer(device, immediateContext, data, size, usage);
	}

	Buffer *D3D11Renderer::CreateIndexBuffer(const void *data, unsigned int size, BufferUsage usage)
	{
		return new D3D11IndexBuffer(device, data, size, usage);
	}

	Buffer *D3D11Renderer::CreateUniformBuffer(const void *data, unsigned int size)
	{
		D3D11UniformBuffer *ubo = new D3D11UniformBuffer(device, immediateContext, data, size, BufferUsage::DYNAMIC);
		ubo->AddReference();
		ubos.push_back(ubo);
		return ubo;
	}

	Buffer *D3D11Renderer::CreateDrawIndirectBuffer(unsigned int size, const void *data)
	{
		D3D11DrawIndirectBuffer *indirect = new D3D11DrawIndirectBuffer(device, immediateContext, data, size);
		indirect->AddReference();
		indirectBuffers.push_back(indirect);
		return indirect;
	}

	Buffer *D3D11Renderer::CreateSSBO(unsigned int size, const void *data, unsigned int stride, BufferUsage usage)
	{
		D3D11SSBO *ssbo = new D3D11SSBO(device, immediateContext, data, size, stride, BufferUsage::STATIC);
		ssbo->AddReference();
		ssbos.push_back(ssbo);

		return ssbo;
	}

	Framebuffer *D3D11Renderer::CreateFramebuffer(const FramebufferDesc &desc)
	{
		Framebuffer *fb = new D3D11Framebuffer(device, immediateContext, desc);
		return fb;
	}

	ShaderProgram* D3D11Renderer::CreateShader(const std::string &vertexName, const std::string &fragmentName, const std::string &defines, const std::vector<VertexInputDesc> &descs, const BlendState &blendState)
	{
		unsigned int id = SID(vertexName + fragmentName + defines);

		// If the shader already exists return that one instead
		if (shaderPrograms.find(id) != shaderPrograms.end())
		{
			return shaderPrograms[id];
		}

		// Else create a new one
		ShaderProgram* shader = new D3D11Shader(device, id, vertexName, fragmentName, defines, descs);
		shaderPrograms[id] = shader;

		return shader;
	}

	ShaderProgram* D3D11Renderer::CreateShader(const std::string &vertexName, const std::string &fragmentName, const std::vector<VertexInputDesc> &descs, const BlendState &blendState)
	{
		return CreateShader(vertexName, fragmentName, "", descs, blendState);
	}

	ShaderProgram* D3D11Renderer::CreateShaderWithGeometry(const std::string &vertexPath, const std::string &geometryPath, const std::string &fragmentPath, const std::string &defines, const std::vector<VertexInputDesc> &descs)
	{
		unsigned int id = SID(vertexPath + geometryPath + fragmentPath + defines);

		// If the shader already exists return that one instead
		if (shaderPrograms.find(id) != shaderPrograms.end())
		{
			return shaderPrograms[id];
		}

		// Else create a new one
		ShaderProgram* shader = new D3D11Shader(device, id, vertexPath, geometryPath, fragmentPath, defines, descs);
		shaderPrograms[id] = shader;

		return shader;
	}

	ShaderProgram* D3D11Renderer::CreateShaderWithGeometry(const std::string &vertexPath, const std::string &geometryPath, const std::string &fragmentPath, const std::vector<VertexInputDesc> &descs)
	{
		return CreateShaderWithGeometry(vertexPath, geometryPath, fragmentPath, "", descs);
	}

	ShaderProgram* D3D11Renderer::CreateComputeShader(const std::string &computePath, const std::string &defines)
	{
		unsigned int id = SID(computePath + defines);

		// If the shader already exists return that one instead
		if (shaderPrograms.find(id) != shaderPrograms.end())
		{
			return shaderPrograms[id];
		}

		// Else create a new one
		ShaderProgram* shader = new D3D11Shader(device, id, computePath, defines);
		shaderPrograms[id] = shader;

		return shader;
	}

	ShaderProgram* D3D11Renderer::CreateComputeShader(const std::string &computePath)
	{
		return CreateComputeShader(computePath, "");
	}

	MaterialInstance *D3D11Renderer::CreateMaterialInstance(ScriptManager &scriptManager, const std::string &matInstPath, const std::vector<VertexInputDesc> &inputDescs)
	{
		MaterialInstance *m = Material::LoadMaterialInstance(this, matInstPath, scriptManager, inputDescs);
		
		materialInstances.push_back(m);

		std::vector<ShaderPass> &shaderPasses = m->baseMaterial->GetShaderPasses();

		// See if we already have states for this material created (because multiple material instances can use the same material with the same state)
		bool found = false;
		for (size_t i = 0; i < states.size(); i++)
		{
			if (states[i].mat == m->baseMaterial)
			{
				found = true;
				/*for (size_t j = 0; j < shaderPasses.size(); j++)
				{
					shaderPasses[j].stateID = static_cast<unsigned int>(i);
				}*/
				return m;
			}
		}

		if (!found)
		{	
			for (size_t i = 0; i < shaderPasses.size(); i++)
			{
				CreateState(shaderPasses[i]);
				shaderPasses[i].stateID = static_cast<unsigned int>(states.size() - 1);
				states[states.size() - 1].mat = m->baseMaterial;
			}
		}

		return m;
	}

	MaterialInstance *D3D11Renderer::CreateMaterialInstanceFromBaseMat(ScriptManager &scriptManager, const std::string &baseMatPath, const std::vector<VertexInputDesc> &inputDescs)
	{
		MaterialInstance *m = Material::LoadMaterialInstanceFromBaseMat(this, baseMatPath, scriptManager, inputDescs);
		materialInstances.push_back(m);

		std::vector<ShaderPass> &shaderPasses = m->baseMaterial->GetShaderPasses();

		// See if we already have states for this material created (because multiple material instances can use the same material with the same state)
		bool found = false;
		for (size_t i = 0; i < states.size(); i++)
		{
			if (states[i].mat == m->baseMaterial)
			{
				found = true;
				/*for (size_t j = 0; j < shaderPasses.size(); j++)
				{
					shaderPasses[j].stateID = static_cast<unsigned int>(i);
				}*/
				return m;
			}
		}

		if (!found)
		{
			for (size_t i = 0; i < shaderPasses.size(); i++)
			{
				CreateState(shaderPasses[i]);
				shaderPasses[i].stateID = static_cast<unsigned int>(states.size() - 1);
				states[states.size() - 1].mat = m->baseMaterial;
			}
		}

		return m;
	}

	Texture *D3D11Renderer::CreateTexture2D(const std::string &path, const TextureParams &params, bool storeTextureData)
	{
		unsigned int id = SID(path);

		// If the texture already exists return that one instead
		if (textures.find(id) != textures.end())
		{
			Texture *tex = textures[id];
			tex->AddReference();
			return textures[id];
		}

		Texture *tex = new D3D11Texture2D(device, immediateContext, path, params, storeTextureData);
		tex->AddReference();
		textures[id] = tex;

		return tex;
	}

	Texture *D3D11Renderer::CreateTexture3D(const std::string &path, const void *data, unsigned int width, unsigned int height, unsigned int depth, const TextureParams &params)
	{
		unsigned int id = SID(path);

		// If the texture already exists return that one instead
		if (textures.find(id) != textures.end())
		{
			Texture *tex = textures[id];
			tex->AddReference();
			return textures[id];
		}

		return nullptr;
	}

	Texture *D3D11Renderer::CreateTextureCube(const std::vector<std::string> &faces, const TextureParams &params)
	{
		unsigned int id = SID(faces[0]);					// Better way? Otherwise if faces[0] is equal to an already stored texture but the other faces are different it won't get loaded

		// If the texture already exists return that one instead
		if (textures.find(id) != textures.end())
		{
			Texture *tex = textures[id];
			tex->AddReference();
			return textures[id];
		}

		return nullptr;
	}

	Texture *D3D11Renderer::CreateTextureCube(const std::string &path, const TextureParams &params)
	{
		unsigned int id = SID(path);

		// If the texture already exists return that one instead
		if (textures.find(id) != textures.end())
		{
			Texture *tex = textures[id];
			tex->AddReference();
			return textures[id];
		}

		return nullptr;
	}

	Texture *D3D11Renderer::CreateTexture2DFromData(unsigned int width, unsigned int height, const TextureParams &params, const void *data)
	{
		// We use a increasing id for textures that are loaded from data because we don't have a path string to convert to ID.
		// So loop until a find a free ID
		while (textures.find(textureID) != textures.end())
		{
			//std::cout << "Conflicting texture ID's\n";
			textureID++;
		}

		Texture *t = new D3D11Texture2D(device, immediateContext, width, height, params, data);
		t->AddReference();
		textures[textureID] = t;
		textureID++;
		return t;
	}

	Texture *D3D11Renderer::CreateTexture3DFromData(unsigned int width, unsigned int height, unsigned int depth, const TextureParams &params, const void *data)
	{
		while (textures.find(textureID) != textures.end())
		{
			textureID++;
		}

		Texture *t = new D3D11Texture3D(device, width, height, depth, params, data);
		t->AddReference();
		textures[textureID] = t;
		textureID++;
		return t;
	}

	void D3D11Renderer::SetDefaultRenderTarget()
	{
		ID3D11RenderTargetView *backBufferRTV = { renderTargetView };
		immediateContext->OMSetRenderTargets(1, &backBufferRTV, depthStencilView);

		immediateContext->ClearRenderTargetView(renderTargetView, clearColor);
		immediateContext->ClearDepthStencilView(depthStencilView, D3D11_CLEAR_DEPTH, 1.0f, 0);

		D3D11_VIEWPORT viewport;
		viewport.Width = static_cast<FLOAT>(width);
		viewport.Height = static_cast<FLOAT>(height);
		viewport.MinDepth = 0.0f;
		viewport.MaxDepth = 1.0f;
		viewport.TopLeftX = 0.0f;
		viewport.TopLeftY = 0.0f;

		immediateContext->RSSetViewports(1, &viewport);
	}

	void D3D11Renderer::SetRenderTarget(Framebuffer *rt)
	{
	}

	void D3D11Renderer::SetRenderTargetAndClear(Framebuffer *rt)
	{
		nullSRVs.clear();

		ClearShaderResources();

		if (rt->AreWritesDisabled() == false)
		{
			D3D11Framebuffer *fb = static_cast<D3D11Framebuffer*>(rt);
			const std::vector<ID3D11RenderTargetView*> rtvs = fb->GetRenderTargetViews();
			ID3D11DepthStencilView *dsv = fb->GetDepthStencilView();

			immediateContext->OMSetRenderTargets(static_cast<UINT>(rtvs.size()), rtvs.data(), dsv);		// Even if we don't have a dsv it will be null so it will act has if we don't have one		

			for (size_t i = 0; i < rtvs.size(); i++)
			{
				immediateContext->ClearRenderTargetView(rtvs[i], clearColor);
			}

			if (dsv)
				immediateContext->ClearDepthStencilView(dsv, D3D11_CLEAR_DEPTH, 1.0f, 0);
		}
		else
		{
			setUAVs = 0;
			nullUAVs.clear();

			for (size_t i = 0; i < imagesToBindUAVs.size(); i++)
			{
				Texture *tex = imagesToBindUAVs[i];

				ID3D11UnorderedAccessView *uav = nullptr;

				if (tex->GetType() == TextureType::TEXTURE2D)
				{
					D3D11Texture2D *tex2d = static_cast<D3D11Texture2D*>(tex);
					uav = tex2d->GetUnorderedAcessView();
				}
				else if (tex->GetType() == TextureType::TEXTURE3D)
				{
					D3D11Texture3D *tex3d = static_cast<D3D11Texture3D*>(tex);
					uav = tex3d->GetUAV();
				}

				immediateContext->OMSetRenderTargetsAndUnorderedAccessViews(0, nullptr, nullptr, setUAVs, 1, &uav, nullptr);

				nullUAVs.push_back(nullptr);
				setUAVs++;
			}
		}

		D3D11_VIEWPORT viewport;
		viewport.Width = static_cast<FLOAT>(rt->GetWidth());
		viewport.Height = static_cast<FLOAT>(rt->GetHeight());
		viewport.MinDepth = 0.0f;
		viewport.MaxDepth = 1.0f;
		viewport.TopLeftX = 0.0f;
		viewport.TopLeftY = 0.0f;

		immediateContext->RSSetViewports(1, &viewport);
	}

	void D3D11Renderer::EndRenderTarget(Framebuffer *rt)
	{
		//immediateContext->OMSetRenderTargets(0, nullptr, nullptr);	
	}

	void D3D11Renderer::EndDefaultRenderTarget()
	{
	}

	void D3D11Renderer::ClearRenderTarget(Framebuffer *rt)
	{
	}

	void D3D11Renderer::SetViewport(const Viewport &viewport)
	{
		D3D11_VIEWPORT v;
		v.Width = static_cast<FLOAT>(viewport.width);
		v.Height = static_cast<FLOAT>(viewport.height);
		v.MinDepth = 0.0f;
		v.MaxDepth = 1.0f;
		v.TopLeftX = static_cast<FLOAT>(viewport.x);
		v.TopLeftY = static_cast<FLOAT>(viewport.y);

		immediateContext->RSSetViewports(1, &v);
	}

	void D3D11Renderer::Submit(const RenderQueue &renderQueue)
	{
		instanceDataOffset = 0;

		instanceDataSSBO->Map();
		char *mappedInstanceData = (char*)instanceDataSSBO->GetMappedPtr();

		glm::mat4 temp;
		for (size_t i = 0; i < renderQueue.size(); i++)
		{
			const RenderItem &ri = renderQueue[i];

			if (ri.transform)
			{
				temp = glm::transpose(*ri.transform);
				memcpy(mappedInstanceData, &temp, sizeof(glm::mat4));
				mappedInstanceData += 64;
			}
			if (ri.meshParams)
			{
				memcpy(mappedInstanceData, ri.meshParams, ri.meshParamsSize);
				mappedInstanceData += ri.meshParamsSize;
			}
		}
		instanceDataSSBO->Unmap();

		for (size_t i = 0; i < renderQueue.size(); i++)
		{
			Submit(renderQueue[i]);
		}
	}

	void D3D11Renderer::Submit(const RenderItem &renderItem)
	{
		const ShaderPass &pass = renderItem.matInstance->baseMaterial->GetShaderPass(renderItem.shaderPass);
		D3D11Shader *shader = static_cast<D3D11Shader*>(pass.shader);
		
		if (currentStateID != pass.stateID)
		{
			const State &s = states[pass.stateID];
			currentStateID = pass.stateID;

			float blendFactor[4] = { 0.0f,0.0f,0.0f,0.0f };

			immediateContext->RSSetState(s.rsState);
			immediateContext->OMSetDepthStencilState(s.dsState, 0);	
			immediateContext->OMSetBlendState(s.blendState, blendFactor, 0xffffffff);
		}

		if (currentShader != shader)
		{
			if(shader->HasGeometry() == false && currentShader && currentShader->HasGeometry() == true)
				currentShader->UnbindGeometry(immediateContext);

			shader->Bind(immediateContext);
			currentShader = shader;
		}

		// Check if it's faster not checking if the topology is different from the previous
		if (currentTopology != pass.topology)
		{
			immediateContext->IASetPrimitiveTopology(static_cast<D3D11_PRIMITIVE_TOPOLOGY>(pass.topology));
			currentTopology = pass.topology;
		}

		setSRVCount = 0;
		unsigned int offset = currentSRVBinding;
		unsigned int samplersOffset = 1;

		/*setSRVs.clear();
		setSamplers.clear();
		*/
		for (size_t i = 0; i < renderItem.matInstance->textures.size(); i++)
		{
			Texture *tex = renderItem.matInstance->textures[i];

			if (!tex)
				continue;

			ID3D11ShaderResourceView *srv = nullptr;
			ID3D11SamplerState *sampler = nullptr;

			if (tex->GetType() == TextureType::TEXTURE2D)
			{
				D3D11Texture2D *tex2d = static_cast<D3D11Texture2D*>(tex);
				srv = tex2d->GetShaderResourceView();
				sampler = tex2d->GetSamplerState();
			}
			else if (tex->GetType() == TextureType::TEXTURE3D)
			{
				D3D11Texture3D *tex3d = static_cast<D3D11Texture3D*>(tex);
				srv = tex3d->GetShaderResourceView();
				sampler = tex3d->GetSamplerState();
			}

			/*setSRVs.push_back(srv);
			setSamplers.push_back(sampler);
			*/
			// Bind all at once instead of one at a time like this
			immediateContext->PSSetShaderResources(offset, 1, &srv);
			immediateContext->PSSetSamplers(samplersOffset, 1, &sampler);
			immediateContext->VSSetShaderResources(offset, 1, &srv);
			immediateContext->VSSetSamplers(samplersOffset, 1, &sampler);
			offset++;
			samplersOffset++;
		}

		// No noticeable difference doing it like this
		/*if (setSRVs.size() > 0)
		{
			immediateContext->PSSetShaderResources(offset, static_cast<UINT>(setSRVs.size()), setSRVs.data());
			immediateContext->PSSetSamplers(samplersOffset, static_cast<UINT>(setSamplers.size()), setSamplers.data());
			immediateContext->VSSetShaderResources(offset, static_cast<UINT>(setSRVs.size()), setSRVs.data());
			immediateContext->VSSetSamplers(samplersOffset, static_cast<UINT>(setSamplers.size()), setSamplers.data());
		}*/

		setSRVCount += offset /*+ static_cast<UINT>(setSRVs.size())*/;

		D3D11VertexArray *vao = static_cast<D3D11VertexArray*>(renderItem.mesh->vao);

		const std::vector<Buffer*> vertexBuffers = vao->GetVertexBuffers();
		const std::vector<VertexInputDesc> &descs = vao->GetVertexInputDescs();
		unsigned int slotOffset = 0;
		for (size_t i = 0; i < vertexBuffers.size(); i++)
		{
			ID3D11Buffer *vb = static_cast<D3D11VertexBuffer*>(vertexBuffers[i])->GetBuffer();
			const unsigned int offset = 0;
			immediateContext->IASetVertexBuffers(slotOffset, 1, &vb, &descs[i].stride, &offset);
			slotOffset++;
		}

		if (renderItem.transform)
		{
			materialDataUBO->Map();
			materialDataUBO->UpdateMapped(&instanceDataOffset, sizeof(unsigned int), 0);
			
			instanceDataOffset++;

			if (renderItem.materialData)
				materialDataUBO->UpdateMapped(renderItem.materialData, renderItem.materialDataSize, renderItem.materialDataSize <= 12 ? sizeof(unsigned int) : 16);			// UBO's have to be 16 byte aligned

			materialDataUBO->Unmap();
		}
		else if (renderItem.materialData)
		{
			materialDataUBO->Map();
			materialDataUBO->UpdateMapped(renderItem.materialData, renderItem.materialDataSize, 0);
			materialDataUBO->Unmap();
		}

		instanceDataOffset += renderItem.meshParamsSize / sizeof(glm::mat4);

		D3D11IndexBuffer *ib = static_cast<D3D11IndexBuffer*>(vao->GetIndexBuffer());
		if (ib)
		{
			immediateContext->IASetIndexBuffer(ib->GetBuffer(), DXGI_FORMAT_R16_UINT, 0);

			if (renderItem.mesh->instanceCount > 0)
				immediateContext->DrawIndexedInstanced(renderItem.mesh->indexCount, renderItem.mesh->instanceCount, 0, 0, renderItem.mesh->instanceOffset);
			else
				immediateContext->DrawIndexed(renderItem.mesh->indexCount, 0, 0);
		}
		else
		{
			immediateContext->Draw(renderItem.mesh->vertexCount, 0);
		}
	}

	void D3D11Renderer::SubmitIndirect(const RenderItem &renderItem, Buffer *indirectBuffer)
	{
	}

	void D3D11Renderer::Dispatch(const DispatchItem &item)
	{
		ClearShaderResources();

		immediateContext->OMSetRenderTargetsAndUnorderedAccessViews(0, nullptr, nullptr, 0, setUAVs, nullUAVs.data(), nullptr);

		const ShaderPass &pass = item.matInstance->baseMaterial->GetShaderPass(item.shaderPass);
		D3D11Shader *shader = static_cast<D3D11Shader*>(pass.shader);

		if (currentShader != shader)
		{
			if (currentShader && currentShader->HasCompute() == false)
			{
				if(currentShader->HasGeometry())
					immediateContext->GSSetShader(nullptr, nullptr, 0);

				immediateContext->VSSetShader(nullptr, nullptr, 0);
				immediateContext->PSSetShader(nullptr, nullptr, 0);
			}

			shader->BindCompute(immediateContext);
			currentShader = shader;
		}
	
		unsigned int offset = currentSRVBinding;
		unsigned int samplersOffset = 1;

		for (size_t i = 0; i < item.matInstance->textures.size(); i++)
		{
			Texture *tex = item.matInstance->textures[i];

			if (!tex)
				continue;

			ID3D11ShaderResourceView *srv = nullptr;
			ID3D11SamplerState *sampler = nullptr;

			const TextureParams &p = tex->GetTextureParams();

			if (tex->GetType() == TextureType::TEXTURE2D)
			{
				D3D11Texture2D *tex2d = static_cast<D3D11Texture2D*>(tex);
				srv = tex2d->GetShaderResourceView();
				sampler = tex2d->GetSamplerState();
			}
			else if (tex->GetType() == TextureType::TEXTURE3D)
			{
				D3D11Texture3D *tex3d = static_cast<D3D11Texture3D*>(tex);
				srv = tex3d->GetShaderResourceView();
				sampler = tex3d->GetSamplerState();
			}

			immediateContext->CSGetShaderResources(offset, 1, &srv);
			immediateContext->CSSetSamplers(samplersOffset, 1, &sampler);

			samplersOffset++;
			offset++;	
		}

		setUAVs = 0;
		nullUAVs.clear();

		for (size_t i = 0; i < imagesToBindUAVs.size(); i++)
		{
			Texture *tex = imagesToBindUAVs[i];

			ID3D11UnorderedAccessView *uav = nullptr;

			if (tex->GetType() == TextureType::TEXTURE2D)
			{
				D3D11Texture2D *tex2d = static_cast<D3D11Texture2D*>(tex);
				uav = tex2d->GetUnorderedAcessView();
			}
			else if (tex->GetType() == TextureType::TEXTURE3D)
			{
				D3D11Texture3D *tex3d = static_cast<D3D11Texture3D*>(tex);
				uav = tex3d->GetUAV();
			}

			immediateContext->CSSetUnorderedAccessViews(setUAVs, 1, &uav, nullptr);

			setUAVs++;
		}

		for (size_t i = 0; i < item.matInstance->buffers.size(); i++)
		{
			Buffer *buf = item.matInstance->buffers[i];
			ID3D11ShaderResourceView *srv = nullptr;

			if (buf->GetType() == BufferType::ShaderStorageBuffer)
			{
				D3D11SSBO *ssbo = static_cast<D3D11SSBO*>(buf);
				srv = ssbo->GetSRV();
			}
			else if (buf->GetType() == BufferType::DrawIndirectBuffer)
			{

			}

			immediateContext->CSSetShaderResources(6 + i, 1, &srv);
		}

		DispatchParams dp = {};
		dp.numWorkGroups = glm::uvec3(item.numGroupsX, item.numGroupsY, item.numGroupsZ);

		dispatchParamsUBO->Update(&dp, sizeof(DispatchParams), 0);

		immediateContext->Dispatch(item.numGroupsX, item.numGroupsY, item.numGroupsZ);

		for (size_t i = 0; i < setUAVs; i++)
			nullUAVs.push_back(nullptr);

		immediateContext->CSSetUnorderedAccessViews(0, setUAVs, nullUAVs.data(), nullptr);		// So the resource can be used on a next pass
	}

	void D3D11Renderer::AddTextureResourceToSlot(unsigned int binding, Texture *texture, bool useStorage, unsigned int stages, TextureInternalFormat viewFormat, bool separateMipViews)
	{
		if (!texture)
			return;

		if (texture->GetType() == TextureType::TEXTURE2D)
		{
			D3D11Texture2D *tex = static_cast<D3D11Texture2D*>(texture);
			ID3D11ShaderResourceView *srv = tex->GetShaderResourceView();
			ID3D11SamplerState *sampler = tex->GetSamplerState();
			immediateContext->VSSetShaderResources(currentSRVBinding, 1, &srv);
			immediateContext->PSSetShaderResources(currentSRVBinding, 1, &srv);

			//immediateContext->vssset
			immediateContext->PSSetSamplers(currentSampler, 1, &sampler);

			currentSRVBinding++;
			currentSampler++;
		}
		/*else if (texture->GetType() == TextureType::TEXTURE3D)
		{
			if (useStorage && !separateMipViews)
			{
				ID3D11UnorderedAccessView *uav = static_cast<D3D11Texture3D*>(texture)->GetUAV();
				immediateContext->OMSetRenderTargetsAndUnorderedAccessViews(0, nullptr, nullptr, currentUAVBinding, 1, &uav, nullptr);
				immediateContext->CSSetUnorderedAccessViews(currentUAVBinding, 1, &uav, nullptr);
				currentUAVBinding++;
			}
		}*/
	}

	void D3D11Renderer::AddBufferResourceToSlot(unsigned int binding, Buffer *buffer, unsigned int stages)
	{
		if (!buffer)
			return;

		if (buffer->GetType() == BufferType::UniformBuffer)
		{
			D3D11UniformBuffer *ubo = static_cast<D3D11UniformBuffer*>(buffer);
			ID3D11Buffer *buf = ubo->GetBuffer();
			immediateContext->VSSetConstantBuffers(currentCBBinding, 1, &buf);
			immediateContext->GSSetConstantBuffers(currentCBBinding, 1, &buf);
			immediateContext->PSSetConstantBuffers(currentCBBinding, 1, &buf);
			currentCBBinding++;
		}
		else if (buffer->GetType() == BufferType::DrawIndirectBuffer)
		{
			D3D11DrawIndirectBuffer *indBuffer = static_cast<D3D11DrawIndirectBuffer*>(buffer);
			if (stages & PipelineStage::COMPUTE)
			{
				ID3D11UnorderedAccessView *uav = indBuffer->GetUAV();
				immediateContext->CSGetUnorderedAccessViews(currentUAVBinding, 1, &uav);
				currentUAVBinding++;
			}
		}
		else if (buffer->GetType() ==BufferType::ShaderStorageBuffer)
		{
			D3D11SSBO *ssbo = static_cast<D3D11SSBO*>(buffer);
			if (stages & PipelineStage::COMPUTE)
			{
				ID3D11UnorderedAccessView *uav = ssbo->GetUAV();
				immediateContext->CSGetUnorderedAccessViews(currentUAVBinding, 1, &uav);
				currentUAVBinding++;
			}
		}
	}

	void D3D11Renderer::SetupResources()
	{
	}

	void D3D11Renderer::UpdateTextureResourceOnSlot(unsigned int binding, Texture * texture, bool useStorage, bool separateMipViews)
	{
	}

	void D3D11Renderer::PerformBarrier(const Barrier &barrier)
	{
	}

	void D3D11Renderer::BindImage(unsigned int slot, unsigned int mipLevel, Texture *tex, ImageAccess access)
	{
		imagesToBindUAVs.push_back(tex);
	}

	void D3D11Renderer::ClearBoundImages()
	{
		imagesToBindUAVs.clear();
	}

	void D3D11Renderer::CopyImage(Texture *src, Texture *dst)
	{
		D3D11Texture2D *srcTex = static_cast<D3D11Texture2D*>(src);
		D3D11Texture2D *dstTex = static_cast<D3D11Texture2D*>(dst);
		immediateContext->CopyResource(dstTex->GetTextureHandle(), srcTex->GetTextureHandle());
	}

	void D3D11Renderer::ClearImage(Texture *tex)
	{
	}

	void D3D11Renderer::UpdateMaterialInstance(MaterialInstance *matInst)
	{
	}

	void D3D11Renderer::ReloadShaders()
	{
	}

	void D3D11Renderer::RebindTexture(Texture *texture)
	{
		/*if (!texture)
			return;

		D3D11Texture2D *tex = static_cast<D3D11Texture2D*>(texture);
		ID3D11ShaderResourceView *srv = tex->GetShaderResourceView();
		ID3D11SamplerState *sampler = tex->GetSamplerState();
		immediateContext->PSSetShaderResources(1, 1, &srv);
		immediateContext->PSSetSamplers(0, 1, &sampler);*/

		//setSRVCount++;

		if (texture)
		{
			ID3D11ShaderResourceView *srv = static_cast<D3D11Texture2D*>(texture)->GetShaderResourceView();
			ID3D11SamplerState *sampler = static_cast<D3D11Texture2D*>(texture)->GetSamplerState();
			immediateContext->PSSetShaderResources(1, 1, &srv);
			immediateContext->PSSetSamplers(0, 1, &sampler);
		}
	}

	/*void D3D11Renderer::SetVoxelTexture(Texture *texture)
	{
		if (texture && texture->GetType() == TextureType::TEXTURE3D)
		{
			ID3D11UnorderedAccessView *uav = static_cast<D3D11Texture3D*>(texture)->GetUAV();
			immediateContext->CSSetUnorderedAccessViews(1, 1, &uav, nullptr);
			this->voxelTexture = texture;
		}
	}

	void D3D11Renderer::SetIndirectBuffer(Buffer *buffer)
	{
		if (buffer && buffer->GetType() == BufferType::DrawIndirectBuffer)
		{
			ID3D11UnorderedAccessView *uav = static_cast<D3D11DrawIndirectBuffer*>(buffer)->GetUAV();
			immediateContext->CSSetUnorderedAccessViews(0, 1, &uav, nullptr);
			this->indirectBuffer = buffer;
		}
	}

	void D3D11Renderer::SetVoxelPosSSBO(Buffer *buffer)
	{
		if (buffer && buffer->GetType() == BufferType::ShaderStorageBuffer)
		{
			ID3D11UnorderedAccessView *uav = static_cast<D3D11DrawIndirectBuffer*>(buffer)->GetUAV();
			immediateContext->CSSetUnorderedAccessViews(1, 1, &uav, nullptr);
			this->voxelPosSSBO = buffer;
		}
	}*/

	void D3D11Renderer::CreateState(const ShaderPass &pass)
	{
		D3D11_RASTERIZER_DESC rsDesc = {};
		rsDesc.FrontCounterClockwise = pass.rasterizerState.frontFace;
		rsDesc.FillMode = D3D11_FILL_SOLID;

		if (pass.rasterizerState.enableCulling == false)
			rsDesc.CullMode = D3D11_CULL_NONE;
		else
			rsDesc.CullMode = static_cast<D3D11_CULL_MODE>(pass.rasterizerState.cullFace);

		rsDesc.DepthBias = false;
		rsDesc.DepthBiasClamp = 0.0f;
		rsDesc.SlopeScaledDepthBias = 0.0f;
		rsDesc.ScissorEnable = false;
		rsDesc.MultisampleEnable = false;
		rsDesc.AntialiasedLineEnable = false;
		rsDesc.DepthClipEnable = true;

		ID3D11RasterizerState *rsState = nullptr;
		device->CreateRasterizerState(&rsDesc, &rsState);

		D3D11_DEPTH_STENCIL_DESC dsDesc = {};
		dsDesc.DepthEnable = pass.depthStencilState.depthEnable;

		if (pass.depthStencilState.depthWrite)
			dsDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
		else
			dsDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
		
		dsDesc.DepthFunc = static_cast<D3D11_COMPARISON_FUNC>(pass.depthStencilState.depthFunc);
		dsDesc.StencilEnable = 0;

		ID3D11DepthStencilState *dsState = nullptr;
		device->CreateDepthStencilState(&dsDesc, &dsState);

		D3D11_BLEND_DESC blendDesc = {};
		blendDesc.AlphaToCoverageEnable = false;
		blendDesc.IndependentBlendEnable = false;
		blendDesc.RenderTarget[0].BlendEnable = pass.blendState.enableBlending;
		blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
		blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
		blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
		blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_SRC_ALPHA;
		blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
		blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;

		if (pass.blendState.enableColorWriting)
			blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
		else
			blendDesc.RenderTarget[0].RenderTargetWriteMask = 0;

		ID3D11BlendState *blendState = nullptr;
		device->CreateBlendState(&blendDesc, &blendState);

		State s = {};
		s.rsState = rsState;
		s.dsState = dsState;
		s.blendState = blendState;

		states.push_back(s);
	}

	bool D3D11Renderer::CreateBackBufferRTVAndDSV()
	{
		if (depthStencilTexture)
			depthStencilTexture->Release();

		if (depthStencilView)
			depthStencilView->Release();

		if (renderTargetView)
			renderTargetView->Release();

		HRESULT hr = swapChain->ResizeBuffers(0, width, height, DXGI_FORMAT_UNKNOWN, 0);

		if (FAILED(hr))
		{
			throw std::runtime_error("Failed to resize swapchain buffers");
			return false;
		}

		ID3D11Texture2D *backBuffer;
		hr = swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&backBuffer));

		if (FAILED(hr))
		{
			throw std::runtime_error("Failed to get backbuffer");
			return false;
		}

		hr = device->CreateRenderTargetView(backBuffer, nullptr, &renderTargetView);

		backBuffer->Release();

		if (FAILED(hr))
		{
			throw std::runtime_error("Failed to create render target view");
			return false;
		}

		// Create depth stencil texture
		D3D11_TEXTURE2D_DESC descDepth = {};
		descDepth.Width = width;
		descDepth.Height = height;
		descDepth.MipLevels = 1;
		descDepth.ArraySize = 1;
		descDepth.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
		descDepth.SampleDesc.Count = 1;
		descDepth.SampleDesc.Quality = 0;
		descDepth.Usage = D3D11_USAGE_DEFAULT;
		descDepth.BindFlags = D3D11_BIND_DEPTH_STENCIL;
		descDepth.CPUAccessFlags = 0;
		descDepth.MiscFlags = 0;

		hr = device->CreateTexture2D(&descDepth, nullptr, &depthStencilTexture);

		if (FAILED(hr))
		{
			throw std::runtime_error("Failed to create depth stencil texture");
			return false;
		}

		// Create depth stencil view
		D3D11_DEPTH_STENCIL_VIEW_DESC descDSV = {};
		descDSV.Format = descDepth.Format;
		descDSV.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
		descDSV.Texture2D.MipSlice = 0;

		hr = device->CreateDepthStencilView(depthStencilTexture, &descDSV, &depthStencilView);

		if (FAILED(hr))
		{
			throw std::runtime_error("Failed to create depth stencil view");
			return false;
		}

		return true;
	}

	void D3D11Renderer::ClearShaderResources()
	{
		setSRVCount++;		// Plus 1 because of the csm texture

		for (size_t i = 0; i < setSRVCount; i++)
			nullSRVs.push_back(nullptr);

		immediateContext->VSSetShaderResources(1, setSRVCount, nullSRVs.data());
		immediateContext->PSSetShaderResources(1, setSRVCount, nullSRVs.data());
	}
}
